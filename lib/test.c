#include "lib/linux.c"

void test(int cond){
    if(cond){
        print("Passed\n", 7);
    } else {
        print("Failed\n", 7);
    }
}

void assert(int cond){
    if(cond == 0){
        print("Assertion failed\n", 16);
        exit(1);
    }
}


