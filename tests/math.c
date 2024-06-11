#include "./lib/test.c"

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

    // SIGFPE {si_signo=SIGFPE, si_code=FPE_INTDIV, si_addr=0x80482b7}
    // c = a / b;
    // test(c == 5);

    return c;
}   