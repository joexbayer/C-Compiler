#include "./lib/test.c"

int main(){
    char a[7];
    int i;

    a[0] = 'P';
    a[1] = 'a';
    a[2] = 's';
    a[3] = 's';
    a[4] = 'e';
    a[5] = 'd';
    a[6] = '\n';

    print(a, 7);

    test(a[0] == 80);
    test(a[1] == 97);
    test(a[2] == 115);
    test(a[3] == 115);
    test(a[4] == 101);
    test(a[5] == 100);
    test(a[6] == 10);
    

    return 0;
}