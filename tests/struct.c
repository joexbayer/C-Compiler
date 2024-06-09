#include "./lib/test.c"

struct data {
    int a;
    int b;
};

struct data* create_data(int a, int b){
    struct data* c;
    
    c = alloc(sizeof(struct data));
    c->a = a;
    c->b = b;

    return c;
}

int pointer(){
    struct data d;
    struct data* p;

    d.a = 1;
    d.b = 2;

    p = &d;

    test(p->a == 1);
    test(p->b == 2);

    return 1;
}

struct data a;
int global(){
    int* p;
    int* q;

    a.a = 1;
    a.b = 2;

    p = 0x8048085;
    q = 0x8048089;

    *p = 1;

    test(*p == 1);
    test(*q == 2);

    test(a.a == 1);
    test(a.b == 2);

    return 1;
}

int local(){
    struct data d;
    d.a = 1;
    d.b = 2;

    test(d.a == 1);
    test(d.b == 2);
    
    return 1;
}

int main(){
    struct data* d;

    pointer();
    global();
    local();

    d = create_data(1, 2);
    assert(d != 0);

    test(d->a == 1);
    test(d->b == 2);

    free(d, sizeof(struct data));

    return 0;
}