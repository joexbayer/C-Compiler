struct math {
    int a;
    int b;
    int c;
};

int add(struct math* m, int a) {
    m->c = a;
    return 0;
}

int main() {
    struct math m1;
    struct math* m2;
    m2 = &m1;
    
    m1.a = 1;
    m1.b = 123;
    m1.c = 0;

    add(m2, 69); 

    return m2->c;
}