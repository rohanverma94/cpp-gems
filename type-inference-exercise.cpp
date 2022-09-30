#include <iostream>
#include <limits>
#include <cmath>
#include <vector>

    template< typename T >
    void type( const T&& ) {
        std::cout << "const T&&" << std::endl;
    }
    template< typename T >
    void type( const T& ) {
        std::cout << "const T&" << std::endl;
    }
    template< typename T >
    void type( T*& ) {
        std::cout << "T*&" << std::endl;
    }
    template< typename T >
    void type( const T*&& ) {
        std::cout << "const T*&&" << std::endl;
    }
    template< typename T >
    void type( const T*& ) {
        std::cout << "const T*&" << std::endl;
    }
    template< typename T, size_t N >
    void type( T val[N] ) {
        std::cout << "T[" << N << "]" << std::endl;
    }
    template< typename T, size_t N >
    void type( const T val[N] ) {
        std::cout << "const T[" << N << "]" << std::endl;
    }
    template< typename T, size_t N >
    void type( T (&val)[N] ) {
        std::cout << "T(&)[" << N << "]" << std::endl;
    }
    template< typename T, size_t N >
    void type( const T (&val)[N] ) {
        std::cout << "const T(&)[" << N << "]" << std::endl;
    }

    int main() {
        type( 1 );   // const T&&
        
        int lvalue1 =2;            
        type(lvalue1); // const T&
        
        int* mvp= new int(2);
        type(mvp); // T*&

        const int* && f {static_cast<int *>(mvp)};
        type(f); //const T* &
        type(std::move(f)); //const T* &&

        

        int arrref[]{1,2};
        type(arrref);
    
    }
