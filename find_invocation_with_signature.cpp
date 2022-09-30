#include <type_traits>
#include <utility>
#include <functional>
#include <cassert>
#include <algorithm>
#include <numeric>
#include <ranges>

template <typename Signature>
struct find_first_invocable_with_signature;

template <typename R, typename... Args>
struct find_first_invocable_with_signature<R(Args...)>
{
    template <typename... Fs>
    constexpr static auto among(Fs&&... funcs)
    {
        return std::get<[]
        {
            int i = 1;
            return (((std::is_invocable_r_v<R, std::decay_t<Fs>, Args...>) ? (i = 0) : i) + ...);
        }()>(std::tuple{std::forward<Fs>(funcs)...});
    }
};

static_assert(
    find_first_invocable_with_signature<int(int)>::among(
        [](int)   { return 0; },
        [](float*){ return 1; }
    )(42) == 0
);

static_assert(
    find_first_invocable_with_signature<int(float*)>::among(
        [](int)   { return 0; },
        [](float*){ return 1; }
    )(nullptr) == 1
);

static_assert(
    find_first_invocable_with_signature<int(int)>::among(
        [](float*){ return 1; },
        [](int)   { return 0; }
    )(42) == 0
);

static_assert(
    find_first_invocable_with_signature<int(float*)>::among(
        [](float*){ return 1; },
        [](int)   { return 0; }
    )(nullptr) == 1
);

int main(){
}

// Compiler Clang
// Flags -std=c++2b -stdlib=libc++

