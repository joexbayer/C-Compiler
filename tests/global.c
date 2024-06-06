#include "./lib/test.c"

char* str;

int main(){
    str = "Passed\n";

    print(str, 7);
    
    return 0;
}