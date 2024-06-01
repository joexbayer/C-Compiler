#include "lib/linux.c"

int main() {
    int i;
    char a[6];
    char* b;

    b = alloc(7);

    a[0] = 'H';
    a[1] = 'e';
    a[2] = 'l';
    a[3] = 'l';
    a[4] = 'o';
    a[5] = ' ';

    b[0] = 'W';
    b[1] = 'o';
    b[2] = 'r';
    b[3] = 'l';
    b[4] = 'd';
    b[5] = 10;
    b[6] = 0;

    while(i < 5){
        print(a, 6);
        print(b, 7);
        i = i + 1;
    }

    free(b, 7);

    return 0;
}