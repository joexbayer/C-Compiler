#include "./lib/test.c"

int evalute(int* i, int j){
    
    test(i == j);
    
    return 0;
}

int main() {
    int j;
    char* ptr;

    ptr = 0;
    j = 0;
    while(j < 10){

        evalute(ptr + j, j);

        j = j + 1;
    }

    return j;
}