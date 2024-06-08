#include "./lib/test.c"

int main(){

    int a;
    a = 10;

    if(a == 10){
        return 0;
    } 

    print("Failed\n", 7);

    return 1;
}
