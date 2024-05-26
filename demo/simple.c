struct math {
    int a;
    int b;
    int c;
};

int add(int a, int b, int c) {
    int r;

    r = a + b + c;

    return r;
}

int perform() {
    int a;
    struct math m;

    a = 20;

    m.a = 6;
    m.b = 10;

    m.c = add(m.a, m.b, a);

    return m.c;
}

int main() {
    int a;

    a = perform();

    return a;
}