#include <stdio.h>

struct math {
    int b;
    int c;
    int a;
};

int add(int a, int b) {
    int r;

    r = a + b + 10;

    return r;
}

int perform() {
    int result;

    result = add(1, 2);

    return result;
}

int main() {
    int a;

    a = perform();

    return a;
}