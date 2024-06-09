// File that tests pointers
#include "lib/linux.c"
#include "lib/test.c"

struct data {
    int a;
    int b;
};

int ext3(struct data* p){
    test(p->a == 1);
    test(p->b == 2);

    p->a = 3;
    test(p->a == 3);

    return 1;
}

int ext2(){
    int a;
    int* p;

    a = 1;
    p = &a;

    test(*p == 1);
    test(p == &a);

    *p = 2;

    test(*p == 2);
    test(a == 2);

    return 1;
}   

int ext(){
    struct data* p;
    struct data d;

    d.a = 1;
    d.b = 2;

    test(d.a == 1);
    test(d.b == 2);

    p = &d;

    test(p->a == 1);
    test(p->b == 2);

    p->a = 3;
    test(p->a == 3);
    test(d.a == 3);

    return 1;
}

int main(){
    int a;
    int b;
    int c;
    int *p;
    int *q;
    int *r;
    struct data d;

    d.a = 1;
    d.b = 2;

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

    ext();
    ext2();
    ext3(&d);

    return 0;
}