#include "./lib/test.c"

int main()
{
    // Test |, &, << and >>

    test(0 | 0 == 0);
    test(0 | 1 == 1);
    test(1 | 0 == 1);
    test(1 | 1 == 1);

    test((0 & 0) == 0);
    test((0 & 1) == 0);
    test((1 & 0) == 0);
    test((1 & 1) == 1);

    test(0 << 0 == 0);
    test(0 << 1 == 0);
    test(1 << 0 == 1);
    test(1 << 1 == 2);

    test(0 >> 0 == 0);
    test(0 >> 1 == 0);
    test(1 >> 0 == 1);
    test(2 >> 1 == 1);

    return 0;
}