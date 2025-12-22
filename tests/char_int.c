#include "./lib/test.c"
#include "./lib/std.c"

int main() {
    char c;
    int i;
    char buf[4];
    char *s;

    c = 'A';
    i = c + 1;
    s = "Hi";

    test(c == 'A');
    test(i == 66);
    test(s[0] == 'H');
    test(*(s + 1) == 'i');

    buf[0] = 'x';
    buf[1] = 'y';
    buf[2] = 0;
    test(buf[0] == 'x');
    test(buf[1] == 'y');

    return 0;
}
