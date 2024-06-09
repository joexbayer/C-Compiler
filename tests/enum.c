#include "./lib/test.c"

enum color {
    RED = 1,
    GREEN,
    BLUE = 5,
};

int main(){
    test(RED == 1);
    test(GREEN == 2);
    test(BLUE == 5);
    return 0;
}