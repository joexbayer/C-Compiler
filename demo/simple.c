#include "lib/linux.c"

int main() {
    int i;
    char* a;

    i = 0;
    a = alloc(4096);

    a[0] = 'H';
    a[1] = 'e';
    a[2] = 'l';
    a[3] = 'l';
    a[4] = 'o';
    a[5] = 10;
    a[6] = 0;

    while(i < 5){

        print(a, 7);

        i = i + 1;
    }

    free(a, 4096);

    return 0;
}