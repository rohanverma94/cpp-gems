// Output:
// fun() & 1
// fun() const & 2
// fun() && 1
// fun() const && 2
// fun() && 3
// fun() && 4
// fun() const & 5
// fun() const && 5

// I have had to write constexpr construtor to get your sample working & its documented as well-
// https://godbolt.org/z/WxsEG99rf

// (1) - fun() && -> Ref-qualifiers with Rvalue reference i.e temporary instance
// and enables to choose move semantics on *this, i.e. this overload will be used only for non-const and Rvalue object

// (2)- fun() & -> Ref-qualifiers with Lvalue reference, default
// one with copy semantics on *this, i.e. this overload will be used only for non-const and Lvalue object

// fun() const & -> Only const access with Lvalue-Ref-qualifiers, refer (2) i.e. this overload will be used only for const and Lvalue object

// fun() const && -> Only const access with Rvalue-Ref-qualifiers, refer (1) i.e. this overload will be used only for const and Rvalue object
//Compiler C++17

#include <iostream> 

/*
Author @ Rohan Verma - https://www.linkedin.com/in/rohan-verma-dbengineer/

fun() && -> Ref-qualifiers with Rvalue reference i.e temporary instance
and enables to choose move semantics on *this

fun() & ->  Ref-qualifiers with Lvalue reference, default 
one with copy semantics on *this

fun() const & -> Only const access with Lvalue-Ref-qualifiers  
fun() const && -> Only const access with Rvalue-Ref-qualifiers
*/
struct S{

constexpr S(int val)noexcept :m(val) {};

int m; 
int fun() & { std::clog << "fun() & " << m << '\n'; return m; } 
int fun() && { std::clog << "fun() && " << m << '\n'; return m; }

int fun() const & { std::clog << "fun() const & " << m << '\n'; return m; } 
int fun() const && { std::clog << "fun() const && " << m <<'\n'; return m; }
};

//CTAD


int main() {
S s{1}; 
s.fun(); // Invoked via const with Lvalue-Ref-qualifiers, fun() & 1
const S cs{2};
cs.fun(); // Invoked via const with Lvalue-Ref-qualifiers, fun() const & 2

std::move(s).fun();  /* Temporary Instance,  
Explicity Invoked via Rvalue-Ref-qualifiers, fun() && 1
*/
std::move(cs).fun(); /* Temporary Instance, 
Explicity Invoked via const with Rvalue-Ref-qualifiers, fun() const && 2
*/
S(3).fun(); /* Temporary Instance 
and compiler chooses to Invoke via Rvalue-Ref-qualifiers, fun() && 3
*/

std::move(S(4)).fun();/*Temporary Instance, We explicity moved 
from constructor to get "Rvalue" then compiler chooses 
to Invoke via Rvalue-Ref-qualifiers, fun() && 4
*/

constexpr S cs2{5}; // called constexpr constructor, so we get a const object
cs2.fun(); // Invoked via const with Lvalue-Ref-qualifiers, fun() const & 5
std::move(cs2).fun(); /* We explicity moved 
from const object to get "Rvalue" and 
to Invoke via const with Rvalue-Ref-qualifiers, fun() const && 5
*/
return 0;
}
