#include <concepts>
#include <cstddef>
#include <limits>
#include <type_traits>

// Type list impl
namespace meta 
{
    template <typename Ty> struct Type_wrapper { using type = Ty; };

    template <typename...TT>
    class Type_list
    {
        template <size_t I = 0, size_t N, size_t C,
            typename Ty, typename...UU>
        static consteval inline auto get_at() {
            static_assert(N < C, "Out of bounds");
            if constexpr(I == N) return Type_wrapper<Ty>{};
            else return get_at<I + 1, N, C, UU...>();
        };

        template <typename U, size_t I = 0, size_t C>
        static consteval inline size_t locate_impl() {
            return std::numeric_limits<size_t>::max();
        };

        template <typename U, size_t I = 0, size_t C,
            typename T, typename...UU>
        static consteval inline size_t locate_impl() {
            if constexpr (std::same_as<U, T>) return I;
            else return locate_impl<U, I + 1, C, UU...>();
        };

        static inline constexpr size_t count_impl = sizeof...(TT);

    public:
        static inline consteval size_t count()
        { return count_impl; };

        template <typename...UU> struct append 
        { using type = Type_list <TT..., UU...>; };

        template <typename...UU> struct prepend 
        { using type = Type_list <UU..., TT...>; };

        template <size_t N>
        static inline consteval auto get()
        {
            return get_at<0, N, Type_list<TT...>::count_impl, TT...>();
        };

        template <typename Ty>
        static inline consteval bool exists()
        {
            return locate_impl<Ty, 0, Type_list<TT...>::count_impl, TT...>() 
                    != std::numeric_limits<size_t>::max();
        };

        template <typename Ty>
        static inline consteval size_t locate()
        {
            return locate_impl<Ty, 0, Type_list<TT...>::count_impl, TT...>();
        };

        template <bool B, typename...UU>
        struct append_if
        { using type = Type_list <TT..., UU...>; };

        template <typename...UU>
        struct append_if <false, UU...>
        { using type = Type_list <TT...>; };
    };

    template <> class Type_list<> 
    {
    public:
        static inline consteval size_t count() { return 0; };

        template <typename Ty>
        static inline consteval bool exists() { return false; };

        template <typename Ty>
        static inline consteval size_t locate()
        { return std::numeric_limits<size_t>::max(); };

        template <typename...UU> struct append 
        { using type = Type_list <UU...>; };

        template <typename...UU> struct prepend 
        { using type = Type_list <UU...>; };

        template <bool B, typename...UU>
        struct append_if
        { using type = Type_list <UU...>; };

        template <typename...UU>
        struct append_if <false, UU...>
        { using type = Type_list <>; };
    };

    template <typename List, typename T, typename...TT>
    consteval inline auto tl_flip(meta::Type_list<T, TT...>)
    {
        return tl_flip<typename List::template prepend<T>::type>(meta::Type_list<TT...>{});
    };

    template <typename List>
    consteval inline auto tl_flip(meta::Type_list<>) { return List{}; };
}

template <typename...TT>
struct tl_flip
{
    using type = decltype(meta::tl_flip<meta::Type_list<>>(meta::Type_list<TT...>{}));
};

template <size_t N> struct Size_t 
{ static inline constexpr size_t value = N; };

template <typename> struct is_idxls : std::false_type {};
template <size_t...NN> 
struct is_idxls <meta::Type_list<Size_t<NN>...>> : std::true_type {};
template <size_t...NN>
struct is_idxls <const meta::Type_list<Size_t<NN>...>> : std::true_type {};
template <> struct is_idxls <meta::Type_list<>> : std::true_type {};

template <typename Ty> concept idxls_c = is_idxls<Ty>::value;



// Token impl
enum Token_vs : size_t
{
    CONSTANT,
    VARIABLE,
    OPERATOR,
    ENDOF,
    WHITESPACE
};

template <char C> struct Token
{
    static inline constexpr char tok = C;
    static inline constexpr auto T_t = Token_vs::VARIABLE;
};

template <> struct Token<'\0'> {
    static inline constexpr char tok = '\0';
    static inline constexpr auto T_t = Token_vs::ENDOF;
};

template <> struct Token<' '> {
    static inline constexpr char tok = ' ';
    static inline constexpr auto T_t = Token_vs::WHITESPACE;
};

template <> struct Token<'+'>  {
    static inline constexpr char tok = '+';
    static inline constexpr auto T_t = Token_vs::OPERATOR;
    static inline constexpr auto expr = 
        [](double _0, double _1) { return _0 + _1; };
};

template <> struct Token<'-'> {
    static inline constexpr char tok = '-';
    static inline constexpr auto T_t = Token_vs::OPERATOR;
    static inline constexpr auto expr = 
        [](double _0, double _1) { return _0 - _1; };
};

template <> struct Token<'*'> {
    static inline constexpr char tok = '*';
    static inline constexpr auto T_t = Token_vs::OPERATOR;
    static inline constexpr auto expr = 
        [](double _0, double _1) { return _0 * _1; };
};

template <> struct Token<'/'> {
    static inline constexpr char tok = '/';
    static inline constexpr auto T_t = Token_vs::OPERATOR;
    static inline constexpr auto expr = 
        [](double _0, double _1) { return _0 / _1; };
};

template <> struct Token<'%'> {
    static inline constexpr char tok = '%';
    static inline constexpr auto T_t = Token_vs::OPERATOR;
    static inline constexpr auto expr = 
        [](double _0, double _1) { return (long)_0 % (long)_1; };
};

#include <cmath>

template <> struct Token<'^'> {
    static inline constexpr char tok = '^';
    static inline constexpr auto T_t = Token_vs::OPERATOR;
    static inline constexpr auto expr = 
        [](double _0, double _1) { return pow(_0, _1); };
};

template <typename> struct is_tokls : std::false_type {};
template <char...TT> 
struct is_tokls <meta::Type_list<Token<TT>...>> : std::true_type {};
template <char...TT> 
struct is_tokls <const meta::Type_list<Token<TT>...>> : std::true_type {};
template <> struct is_tokls <meta::Type_list<>> : std::true_type {};

template <typename Ty> concept tokls_c = is_tokls<Ty>::value;



// String interning : AlexPolt (http://alexpolt.github.io/intern.html)
#define N3599
namespace intern
{
  template<char... NN> struct string {
    static constexpr char const value[ sizeof...(NN) ]{NN...};
    using tokens = meta::Type_list<Token<NN>...>;
    static inline constexpr auto data() { return value; }
  };

  template<char... N> constexpr char const string<N...>::value[];
  template<typename T> struct is_string { static const bool value = false; };
  template<char... NN> struct is_string< string<NN...> > { static const bool value = true; };
}

template <typename String> concept string_c = intern::is_string<String>::value;

#include <utility>

template <size_t N>
struct const_string {
    consteval const_string(const char (&str)[N]) { std::copy(str, str + N, string); }
    static inline constexpr size_t value = N;
    char string[N];
};

template <const_string S, size_t...Is> 
constexpr auto cs_to_s(std::integer_sequence<size_t, Is...>) 
{ return intern::string<S.string[Is]...>{}; };

template <const_string S> 
constexpr auto make_str_t() 
{ return cs_to_s<S>(std::make_index_sequence<decltype(S)::value>{}); };



// Lexer / Tokenization
template <tokls_c Vs, tokls_c Os, idxls_c Is>
struct Lex_dict
{
    using Vars = Vs;
    using Ops = Os;
    using Idxs = Is;

    using flip = 
        Lex_dict<Vs,
        decltype(meta::tl_flip<meta::Type_list<>>(Os{})),
        decltype(meta::tl_flip<meta::Type_list<>>(Is{}))>;
};

using Empty_lex = Lex_dict<meta::Type_list<>, meta::Type_list<>, meta::Type_list<>>;

template <typename> struct is_lxdc : std::false_type {};
template <tokls_c Vs, tokls_c Os, idxls_c Is> 
struct is_lxdc <Lex_dict<Vs, Os, Is>> : std::true_type {};
template <tokls_c Vs, tokls_c Os, idxls_c Is>
struct is_lxdc <const Lex_dict<Vs, Os, Is>> : std::true_type {};

template <typename Ty> concept lxdc_c = is_lxdc<Ty>::value;



// Lexer append impl
namespace meta 
{
    template <lxdc_c, char, size_t> struct Lex_append;

    template <lxdc_c Lex, char C> struct Lex_append <Lex, C, Token_vs::VARIABLE>
    {
        static inline constexpr bool do_append 
            = not (Lex::Vars::template exists<Token<C>>());

        static inline constexpr size_t idx_v = 
            (do_append) ? Lex::Vars::count() : 
            Lex::Vars::template locate<Token<C>>();

        using type = Lex_dict<
            typename Lex::Vars::template append_if<do_append, Token<C>>::type,
            typename Lex::Ops, 
            typename Lex::Idxs::template append<Size_t<idx_v>>::type
        >;
    };

    template <lxdc_c Lex, char C> struct Lex_append <Lex, C, Token_vs::OPERATOR>
    {
        using type = Lex_dict<
            typename Lex::Vars, 
            typename Lex::Ops::template append<Token<C>>::type, 
            typename Lex::Idxs
        >;
    };

    template <lxdc_c Lex, char C> struct Lex_append <Lex, C, Token_vs::WHITESPACE>
    { using type = Lex; };

    template <lxdc_c Lex, char C> struct Lex_append <Lex, C, Token_vs::ENDOF>
    { using type = Lex; };

}

template <lxdc_c, typename> struct Lex_append;
template <lxdc_c Lex, char C> struct Lex_append <Lex, Token<C>>
{
    using type = typename meta::Lex_append<Lex, C, Token<C>::T_t>::type; 
};

class Tokenize
{
    template <lxdc_c Lex>
    static inline consteval auto 
        gen_tokens_impl(meta::Type_list<>)
    {
        return Lex{};
    };

    template <lxdc_c Lex, char C, char...CC>
    static inline consteval auto 
        gen_tokens_impl(meta::Type_list<Token<C>, Token<CC>...>)
    {
        return gen_tokens_impl
            <typename Lex_append<Lex, Token<C>>::type>
            (meta::Type_list<Token<CC>...>{});
    };

public:
    template <string_c Expr>
    static consteval auto gen_tokens()
    {
        return gen_tokens_impl<Empty_lex>(typename Expr::tokens{});
    };
};



// Pointer attributes
namespace meta
{
    template <typename> struct Pointer_attributes {};

    template <typename R, typename...Args>
    struct Pointer_attributes <R(*)(Args...)>
    {
        using Ret_t = R;
        using Arg_t = Type_list<Args...>;
        static constexpr inline size_t count = sizeof...(Args);
    };

    template <typename R, class C, typename...Args>
    struct Pointer_attributes <R(C::*)(Args...) const>
    {
        using Ret_t = R;
        using Class_t = C;
        using Arg_t = Type_list<Args...>;
        static constexpr inline size_t count = sizeof...(Args);
    };
}

template <auto Lam>
struct Lambda_wrapper 
    : meta::Pointer_attributes<decltype(&decltype(Lam)::operator ())>
{
    static constexpr inline auto function = Lam;
};

template <auto...Lams>
struct Lambda_list
{
    using value = meta::Type_list<Lambda_wrapper<Lams>...>;
    static constexpr inline size_t count = sizeof...(Lams);
    static constexpr inline size_t arg_count = (Lambda_wrapper<Lams>::count + ...) - 1;
};



#include <tuple>

// Lambda constructor
namespace meta
{
    template <auto, typename Input>
    struct Unpack_as
    { using type = Input; };

    template <auto L, size_t _1, size_t _2>
    struct Final
    {
        template <typename...TT>
        constexpr static inline auto call(std::tuple<TT...>& t)
        {
            return L(std::get<_1>(t), std::get<_2>(t));
        };
    };

    template <typename, typename> struct Expand;

    template <auto L, auto...LL, size_t N, size_t...NN>
    struct Expand <Lambda_list<L, LL...>, Type_list<Size_t<N>, Size_t<NN>...>>
    {
        template <typename...TT>
        constexpr static inline auto call(std::tuple<TT...>& t)
        {
            if constexpr(sizeof...(NN) == 1) return Final<L, NN..., N>::call(t);
            else return L(
                Expand<Lambda_list<LL...>, 
                Type_list<Size_t<NN>...>>::call(t), 
                std::get<N>(t)
            );
        };
    };
}

template <char...VV, char...OO, size_t...NN>
consteval inline auto 
get_as_lambda(
    Lex_dict<
        meta::Type_list<Token<VV>...>, 
        meta::Type_list<Token<OO>...>,
        meta::Type_list<Size_t<NN>...>>)
{
    using lambda_pack = Lambda_list<Token<OO>::expr...>;
    using index_pack = meta::Type_list<Size_t<NN>...>;
    using tuple_in = std::tuple<typename meta::Unpack_as<VV, double>::type&&...>;

    return [] (tuple_in t) constexpr
    {
        return meta::Expand<lambda_pack, index_pack>::call(t);
    };
};


// Expression builder
#include <string_view>

template <const_string S, string_c Expr = decltype(make_str_t<S>())> 
struct Eval
{
protected: static inline constexpr std::string_view s_name = Expr::value;
public:
    template <typename...TT>
    constexpr double operator()(TT&&...tt) const
    {
        using flipped = typename decltype(Tokenize::gen_tokens<Expr>())::flip;
        constexpr auto e = get_as_lambda(flipped{});
        return e(std::make_tuple<TT&&...>(static_cast<TT&&>(tt)...));
    };

    std::string_view name() const { return Eval<S, Expr>::s_name; }
};

#include <iostream>

int main()
{
    constexpr double d = Eval<"a - b * c / d">{}(1.0, 2.0, 7.0, 3.9);
    std::cout << d << std::endl;

    auto e = Eval<"g ^ b">{};
    std::cout << e(3.1, 4.7) << std::endl;
}

//Compiler GCC12.2 
//Flags -std=c++2b -O3
