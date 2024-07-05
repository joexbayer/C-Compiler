#include "./lib/test.c"

int main(){
    char* test;
    int res;

    test = "Test\n";
    res = test[0] == 84;
    res = test[1] == 101;

    return 0;
}