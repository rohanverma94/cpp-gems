struct CRAZY_PANTS{
    float pants[10];
};

//Bigger pants really quick! 1000s of redbulls at once 
CRAZY_PANTS runtime_make_pants_big(CRAZY_PANTS x, CRAZY_PANTS y){
    CRAZY_PANTS result{};

    #pragma omp simd 
    for(int INDEX = 0; INDEX < 10; INDEX++){ 
        result.pants[INDEX] = x.pants[INDEX] + y.pants[INDEX];
    }
    return result;
}

//Grandma making your pants bigger, slow...
constexpr CRAZY_PANTS make_pants_big(CRAZY_PANTS x, CRAZY_PANTS y){
     if (__builtin_is_constant_evaluated()) {
        CRAZY_PANTS result{};
       
       for(int INDEX = 0; INDEX < 10; INDEX++){ 
        result.pants[INDEX] = x.pants[INDEX] + y.pants[INDEX];
        }
        return result;
    }else{
        return runtime_make_pants_big(x,y);
    }
}


//Compile time stuff!
    constexpr CRAZY_PANTS enlarge_pants_compile_time = make_pants_big(
        CRAZY_PANTS{2.0,9.0,1.3,1.2,0.0,1.3,1.2,5.0,1.3,1.2},
    CRAZY_PANTS{1.2,0.0,1.3,1.2,0.0,1.3,1.2,6.0,1.3,1.2});

    CRAZY_PANTS output[(unsigned)enlarge_pants_compile_time.pants[0]] = 
    {enlarge_pants_compile_time};

    //Check "output" label in assembly window

//Runtime stuff!
    CRAZY_PANTS set1{1.2,0.0,1.3,1.2,0.0,1.3,1.2,0.0,1.3,1.2},
    set2{1.2,0.0,1.3,1.2,0.0,1.3,1.2,0.0,1.3,1.2};

    CRAZY_PANTS res_plain {make_pants_big(set1,set2)};


