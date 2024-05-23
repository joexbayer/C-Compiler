struct math {
    int b;
    int c;
    int a;
};

int add(int a, int b) {
    return a + b;
}

struct math m;

int main() {
    

    m.a = add(10, 10);

    if(m.a == 10) {
        printf("m.a is 10\n");
    } else if (m.a == 20) {
        printf("m.a is 20\n");
    } else {
        printf("m.a is neither 10 nor 20\n");
    }

    printf("Value of a: %d\n", m.a);

    return 0;
}