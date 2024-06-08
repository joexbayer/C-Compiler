#include "./lib/test.c"

int main(){

    int a;

    a = 10;

    while (a > 0){
        a = a - 1;
    }

    test(a == 0);

    while(a < 10){
        a = a + 1;
    }

    test(a == 10);

    return 0;
}