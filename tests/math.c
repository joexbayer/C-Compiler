#include "lib/test.c"

// File that tests math operations
int main(){
    int a;
    int b;
    int c;

    a = 10;
    b = 2;
    c = 0;

    c = a + b;
    test(c == 12);

    c = a - b;
    test(c == 8);

    c = a * b;
    test(c == 20);

    //c = a / b;
    //test(c == 5);

    return c;
}   