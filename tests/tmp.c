#include "./lib/test.c"


int main() {
    char a[7];
    int i;

    a[0] = 'P';
    a[1] = 'a';

    i = a[0];

    test(i == 80);
    return 0;
}