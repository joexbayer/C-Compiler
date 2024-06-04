#include "lib/linux.c"

char* str;

void test(int cond){

    str = "Passed\n";

    if(cond){
        print(str, 7);
    } else {
        print("Failed\n", 7);
    }
}

void assert(int cond){
    if(!cond){
        print("Assertion failed\n", 16);
        exit(1);
    }
}


