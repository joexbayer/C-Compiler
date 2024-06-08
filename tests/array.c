#include "./lib/test.c"


int main(){
    char a[7];

    a[0] = 'P';
    a[1] = 'a';
    a[2] = 's';
    a[3] = 's';
    a[4] = 'e';
    a[5] = 'd';
    a[6] = '\n';

    print(a, 7);
    return 0;
}