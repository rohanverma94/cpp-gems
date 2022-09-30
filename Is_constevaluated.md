# The confusion with `std::is_constant_evaluated`, hopefully cleared up
**Disclaimer:** I am not on the committee, and while I have done my due diligence, I could easily
have missed something. This is also not an attack on anyone. As stated the confusion is quite
understandable, and the talk mentioned generally does a good job of introducing some small features
of C++20, even if it does contain a minor mistake.

## Introduction
This is inspired by Timur Doumler's
[C++20: The small things](https://www.youtube.com/watch?v=Xb6u8BrfHjw) given at CppCon 2019 where,
at [48:00](https://www.youtube.com/watch?v=Xb6u8BrfHjw&t=48m00s), somebody points out a mistake on
his slide.

In short, given
```cpp
#include <type_traits>

int __magic_fast_square(int);

constexpr int square(int i){
    if(std::is_constant_evaluated()){
        return i*i;
    }else{
        return __magic_fast_square(i);
    }
}
```
where `__magic_fast_square` is an invented intrinsic function that cannot be evaluated at compile
time, what does `square(3)` do? More specifically which path does it take?

Timur says he believes it should take the first branch. This is wrong (or at the very least
misleading), but completely non-obvious to many people. I will attempt to explain why this is false,
and hopefully help clear up the confusion. I will also note some other corner cases and how to deal
with them.

## What is `std::is_constant_evaluated`?
[`std::is_constant_evaluated`](https://en.cppreference.com/w/cpp/types/is_constant_evaluated) is a
function, expected to be available in C++20, which allows you to detect if an expression is
evaluated at compile time.

The main (and pretty much only) usecase, is to let the programmer to pick between two different
implementations of a function, one that is fast for runtime, and one that is `constexpr` friendly
for compile time.<sup>[1](#endnote-1)</sup>

## The misconception
The big misconception is that a constant expression (such as a call to a `constexpr` function) is
supposed to be evaluated at compile time, if all the inputs are known at compile time. This is
simply not true, but previously it was hard to tell.

Take a look at
```cpp
constexpr int add(int a,int b){
    return a+b;
}

int add_test1(){
    int r = add(1,2);
    return r;
}
```

Is `r` computed at compile time? Conceptually the answer is yes, and if you compiled the code with
optimisations enabled, most compilers would indeed precompute the result, however I will posit that,
no, it is in fact not.

Instead what has happened, is the compiler produced the runtime code to call `add` with `1` and `2`
as arguments, then the optimiser ran, and inlined the call, proceded to do constant folding,
which resulted in simply returning `3`.

To properly compute `r` at compile time, you would write
```cpp
int add_test2(){
    constexpr int r = add(1,2);
    return r;
}
```

## `std::is_constant_evaluated` breaks that assumption
Previously there was no perceptible difference between the two, but `std::is_constant_evaluated`
changes that, by literally letting you ask the compiler if the call is evaluated at compile
time.<sup>[2](#endnote-2)</sup> As a result the following
```cpp
int square_test1(){
    int r = square(3);
    return r;
}
```
is *required* to take the second branch in `square` and call `__magic_fast_square`. If it were an
intrinsic function, the optimiser might constant fold it, just as with `add` in the previous
example, or it might not.<sup>[3](#endnote-3)</sup>

Similarly the following
```cpp
int square_test2(){
    constexpr int r = square(3);
    return r;
}
```
is *required* to take the first branch, and be evaluated at compile time. The same is true for other
contexts that require a constant expression, such as `template` arguments, array bounds, `case`
labels, calls to `consteval` functions, etc. I will such a context a *constant
context*.<sup>[4](#endnote-4)</sup>

## Interaction with `constexpr` functions
We have looked at how a `constexpr` variable forces the evaluation to be done at compile time,
but what about `constexpr` functions, like the following
```cpp
constexpr int square3(){
    int r = square(3);
    return r;
}
```
which looks suspiciously similar to `square_test1`, which if you remember didn't compute `r` at
compile time. The only difference is that this one is `constexpr`.

So, how does `constexpr` affect it? Is `r` computed at compile time now? It depends on the how
`square3` is called. For example the following
```cpp
int square3_test1(){
    int r2 = square3();
    return r2;
}
```
behaves exactly the same way as `square_test1`, computing `r` and `r2` at runtime, but
```cpp
int square3_test2(){
    constexpr int r2 = square3();
    return r2;
}
```
is different, since `square3` is called in a *constant context*, that information propagates down
to the call to `square`, which now takes the first branch. Both `r` and `r2` are computed at
compile time, although only the latter is usable in a *constant context* since it is the only one
actually marked `constexpr`.

You can think of this as all the local variables in a `constexpr` function inheriting the ability
to be computed at compile time from the function, and will be computed at compile time, when the
function is called in a *constant context*.

I believe this also helps explain why `std::is_constant_evaluated` should not be used directly
inside of `if constexpr`. You are essentially forcing that call to be evaluated at compile time,
as with `square_test2`, which is exactly what isn't the point. If you instead use a regular `if`,
the call inherits the ability to be evaluated at compile time, just as if you had assigned it to a
variable.

## Interaction with `const`
The previous section talked about contexts where one or the other was required, but there are contexts
where the compiler might have to choose.

One such context is with using the `const` keyword. In C++98 `constexpr` was not a thing yet, but
we already disliked macros enough to want an alternative. Thus when the value of a `const`
variable<sup>[5](#endnote-5)</sup> was known at compile time, they were allowed to be used in a
*constant context*.

This has carried over to C++11 where the two allow you to do pretty much the same things. The only
difference between a `const` variable and a `constexpr` one, is that the latter is *required* to
be computed at compile time, or otherwise fail to compile (ie. is a *constant context*).

As a result
```cpp
int square_test3(){
    const int r = square(3);
    return r;
}
```
is going to be evaluated at compile time, but
```cpp
int three = 3;

int square_test4(){
    const int r = square(three);
    return r;
}
```
cannot be.

In `square_test3`, you would be able to use `r` in a *constant context*, in `square_test4`
you wouldn't.

The compiler tries to compute it at compile time, in which case `std::is_constant_evaluated` would
return `true`. If that fails, however, the compiler backtracks and generate the runtime code, and
`std::is_constant_evaluated` returns `false`.

This leads to an interesting case
```cpp
constexpr int bad_square(int i){
    if(std::is_constant_evaluated()){
        return __magic_fast_square(i);
    }else{
        return i*i;
    }
}

int bad_square_test1(){
    const int r = bad_square(3);

    // std::integral_constant<int,r>();

    return r;
}
```
where the only computations inside `bad_square`, a call to `std::is_constant_evaluated` and
multiplying `i` by itself, all of which can be evaluated at compile time, but since the `true`
branch could not, `std::is_constant_evaluated` had to return `false`, and that poisoned the whole
thing. As a result `r` *cannot* be used in a *constant context*, like the one in the comment.

Making `r` `constexpr` as in
```cpp
// int bad_square_test2(){
//     constexpr int r = bad_square(3);
//     return r;
// }
```
fixes this and the code would not compile, and you would notice the bug.

## Sometimes the compiler must choose
Building on the previous section, what if a function is complicated and takes a long time to
evaluate?

Consider the sum of [total stopping times](https://en.wikipedia.org/wiki/Collatz_conjecture#Statement_of_the_problem) of the Collatz conjecture:
```cpp
constexpr int collatz_time(int n){
    int r = 0;
    for(;n > 1;++r){
        if(n%2 == 0){
            n = n/2;
        }else{
            n = 3*n+1;
        }
    }

    return r;
}

constexpr int sum_collatz_time_const(int n){
    int r = 0;
    for(int i = 1;i < n;++i){
        r += collatz_time(i);
    }

    return r;
}

int sum_collatz_time_fast(int n);

constexpr int sum_collatz_time(int n){
    if(std::is_constant_evaluated()){
        return sum_collatz_time_const(n);
    }else{
        return sum_collatz_time_fast(n);
    }
}
```

Since the collatz conjecture is, as of writing this, unproven, no one knows if the functions will
`return`, for all `n`.<sup>[6](#endnote-6)</sup>

Now we try
```cpp
int collatz_test1(){
    const int r = sum_collatz_time(100000);
    return r;
}
```
and ask the question: Is `r` computed at compile time? Can we use it in a *constant context*? The
answer to that, is it depends on the compiler and the compilation flags used.<sup>[7](#endnote-7)

The result exists (it is `10753712`) and it can be computed at compile time, but at some point `n`
becomes too large, and the compiler gives up, but since it also cannot prove it is *undefined
behaviour* (an infinite loop would be), it *has* to evaluating it at runtime, resulting in `r` not
being usable in a *constant context*, despite the fact that if it had just tried a little
longer, it very well could have been.<sup>[8](#endnote-8)</sup>

The takeaway here is the difference between `int a = ...;`, `const int b = ...;` and
`constexpr int c = ...;`: `a` is *never* computed at compile time; `b` *might* be, if the compiler
can figure it out and `c` *always* is.

More importantly, for `a` and `b`, the compiler is *required to compile* the code, *unless* it can
prove that the code contains undefined behaviour (which for the collatz case involves solving a, so
far, unsolved problem). This is unlike `c`, where it is *required to fail*, *unless* it can computed
it at compile time.

## Interaction with static initialisation
There is another context that *might* be evaluated at compile time, namely variables with
[*static/thread storage duration*](https://en.cppreference.com/w/cpp/language/storage_duration),<sup>[9](#endnote-9)</sup> and this too interacts with `std::is_constant_evaluated`.

I believe this was, in fact, one of the main reasons behind `constexpr`: It was supposed to help
deal with the
[static initialisation order fiasco](https://isocpp.org/wiki/faq/ctors#static-init-order), by
allowing some objects, those with a `constexpr` constructor, to be initialised at compile time, and
thus letting other objects depend on them. This is why
[`std::mutex`](https://en.cppreference.com/w/cpp/thread/mutex) has a
[`constexpr` constructor](https://en.cppreference.com/w/cpp/thread/mutex/mutex).

This interaction is similar to that of `const` variables:

```cpp
int square_test5 = square(3);
```
is going to be initialised at compile time, while
```cpp
int square_test6 = square(three);
```
won't, and
```cpp
int collatz_test2 = sum_collatz_time(100000);
```
might.

The result is unchanged when you use `static`/`extern` and/or a combination of
`thread_local`/`inline`.

C++20 also comes with a new keyword
[`constinit`](https://en.cppreference.com/w/cpp/language/constinit), which forces variables to be
initialised at compile time (like `square_test2`), while still being
mutable.<sup>[10](#endnote-10)</sup> As a result
```cpp
constinit int square_test7 = square(3);
```
is *required* to be initialised at compile time, while
```cpp
// constinit int square_test8 = square(three);
```
won't compile, and
```cpp
// constinit int collatz_test3 = sum_collatz_time(100000);
```
either must, or will fail to compile, if the compiler gives up (which it probably will, hence the comment).

## In defence of this mess
This is the where I feel many would criticise the committee for all the unnecessary complexity that
has seeped into the language:

Three different cases that all behave subtly differently, one of where
compilers are pretty much required to differ in their implementation, and sometimes (*static/thread storage duration*) it looks like one case, but is another.

As such, this is where I will try to preemptively defend them, and make the case that this falls
into the category of essential complexity, at least given the history of the language:

First of all, assuming you write your `constexpr` functions correctly, your program will behave the
same no matter which branch is taken. The only difference will be in the code generated by the
compiler.

Second, if the compiler didn't take your `constexpr` friendly branch, and this resulted in worse
codegen, you would either not notice, hence it probably didn't matter anyway, or you do notice,
because it is a performance problem, in which case it is an easy fix, once a member of your team
knows what to look for.

Third, since the two most common cases are standardised, if you do have a logic error, say a
poorly written `constexpr` friendly branch, different compilers are *required* to behave the same
way, hence letting you use their debugging tools, instead of one compiler taking the runtime path, and
the other noticing it could take the `constexpr` friendly path and making your bug surface.

Fourth, the complexity from `const` and *static/thread storage duration* stems from solving actual
problems. The static initialisation order fiasco was a big enough problem for people to invent
`constexpr`, to help with it, and this path has worked well enough, that people have spent time and
money to implement first [`require_constant_initialization`](https://clang.llvm.org/docs/AttributeReference.html#require-constant-initialization-constinit-c-20) in Clang, and now get
`constinit` standardised.

Fifth, you cannot solve the halting problem. Even if you solved the Collatz conjecture, you
cannot solve all programs, so always expecting the compiler to try to evaluate everything at compile
time is silly.

And lastly, it is a checkable issue. I don't yet know of any such tools, but finding `constexpr`
functions called with constant parameters does sound like something a tool could do. Similarly
suggesting uses of `constinit` is almost certainly something a tool could do, since the compiler
already tries to do initialise them this way.

That said, I admit it is complicated and non-obvious, and I don't expect this to clear everything up
for everyone, so [here is a Godbolt link](https://godbolt.org/z/XpCJ10) where you can play around
with the examples, and see how they behave when you change them. You can click the arrows next to
the compilers, to see the assembly output. MSVC does not yet support the features discussed herein.

## Conclusion
`std::is_constant_evaluated` has some pitfalls, that subverts how many programmers thought constant
evaluation worked, due to misconceptions that were previously not observable.

I posit that these pitfalls can be avoided by following a set of guidelines:
1. When you expect a value to be computed at compile time, *always* say so by assigning it to a
   `constexpr` variable.  
   *Do not* assume the compiler will just do it for you.
2. A non `constexpr` variable, means that it *shouldn't*, be computed at compile time.  
   If it can be, put a comment to make clear it is intentional, and get someone else to review it.
3. Use `const` variables *only* when you know they won't change, but where the value cannot be
   computed at compile time.  
   If it can *always* use a `constexpr` variable instead.<sup>[11](#endnote-11)</sup>  
   If you don't want it to be use a non `const` variable.
4. Use `constinit`, when possible, to force variables with *static/thread storage duration* to be
   initialised at compile time.  
   *Do not* assume the compiler will always do it for you.

Hopefully, tools will be available in the future, to help us enforce these guidelines.

---
<a id="endnote-1"></a> 1: An example of where this would be useful is with MSVC's
[`_BitScanReverse`](https://docs.microsoft.com/en-us/cpp/intrinsics/bitscanreverse-bitscanreverse64)
intrinsic. A `constexpr` friendly implementation would use a loop, while the intrinsic compiles to a
single `bsr` instruction at runtime.

<a id="endnote-2"></a> 2: The standard calls this *manifestly constant-evaluated*.
See [N4830](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/n4830.pdf) \[expr.const\]/13,
for the full details.

<a id="endnote-3"></a> 3: As far as I can tell MSVC does not constant fold `_BitScanReverse`.

<a id="endnote-4"></a> 4: *Constant context* is a term I have made up for this.

<a id="endnote-5"></a> 5: Note the difference between `const int* p1;` and `int* const p2;`:
`p2` is a `const` variable, while `p1` isn't. Since references are essentially equivalent to the
latter they are `const` variables too, even if the object they reference isn't.

<a id="endnote-6"></a> 6: While we do know that it does for all values that fit in an `int`, the
compiler probably won't have time to prove it, and definitely not in the general case, of a custom
`constexpr` bignum.

<a id="endnote-7"></a> 7: The flags for the major compilers can be found at the following links: [GCC](https://gcc.gnu.org/onlinedocs/gcc/C_002b_002b-Dialect-Options.html#index-fconstexpr-depth),
[Clang](https://clang.llvm.org/docs/UsersManual.html#cmdoption-fconstexpr-depth)
and
[MSVC](https://docs.microsoft.com/en-us/cpp/build/reference/constexpr-control-constexpr-evaluation).

<a id="endnote-8"></a> 8: Interestingly, with GCC I was able to find values for `n` where
`sum_collatz_time_const` could not be evaluated at compile time, but where the optimiser would
still fold the runtime code into a constant. I have not checked other compilers for this.

<a id="endnote-9"></a> 9: Note *static storage duration*, instead of `static` *storage duration*.
This is because `static` is not always required, and on global variables (which always have
*static/thread storage duration*) it instead specifies internal linkage, which, ironically, makes
the static initialisation order fiasco easier to deal with. You can blame C for this.

<a id="endnote-10"></a> 10: Strangely enough `constinit` is not allowed on local variables in
functions, which seems like an oversight to me. You can work around this by first making a
`constexpr` variable, and then copying it to a different variable. In this regard `constexpr` would
be the same as a `constinit` `const` variable.

<a id="endnote-11"></a> 11: Although it sounds iffy, I suppose it is possible to have a `static`
member variable in a `class` `template` where you don't if it can be computed at compile time,
before it is instantiated. Such a case it is probably worth a comment, and a thorough code review.
