#include "./lib/test.c"

int main(){

    int a;

    a = 10;
    if (a == 10){
        test(1);
    } else {
        test(0);
    }

    if (a == 11){
        test(0);
    } else {
        test(1);
    }

    if (a != 10){
        test(0);
    } else {
        test(1);
    }

    if (a != 11){
        test(1);
    } else {
        test(0);
    }

    if (a < 11){
        test(1);
    } else {
        test(0);
    }

    if (a < 10){
        test(0);
    } else {
        test(1);
    }

    if (a <= 10){
        test(1);
    } else {
        test(0);
    }

    if (a <= 9){
        test(0);
    } else {
        test(1);
    }

    if (a > 9){
        test(1);
    } else {
        test(0);
    }

    if (a > 10){
        test(0);
    } else {
        test(1);
    }

    a = -1;
    if (a < 0){
        test(1);
    } else {
        test(0);
    }

    return 0;
}