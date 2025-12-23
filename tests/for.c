#include "./lib/test.c"

int main() {
    int i;
    int sum;
    sum = 0;

    for (i = 0; i < 5; i = i + 1) {
        sum = sum + i;
    }

    test(sum == 10);
    return 0;
}
