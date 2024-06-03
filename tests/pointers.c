// File that tests pointers
#include "lib/linux.c"
#include "lib/test.c"

int main(){
    int a;
    int b;
    int c;
    int *p;
    int *q;
    int *r;

    a = 10;
    b = 2;
    c = 0;

    p = &a;
    test(*p == 10);

    q = &b;
    test(*q == 2);

    r = &c;
    test(*r == 0);

    *r = *p + *q;
    test(*r == 12);

    *r = *p - *q;
    test(*r == 8);
    *r = *p * *q;
    test(*r == 20);
    //*r = *p / *q;
    //test(*r == 5);

    return 0;
}