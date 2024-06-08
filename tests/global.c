#include "./lib/test.c"

char* str;
char arr[7];

int main(){
    
    str = "Passed\n";
    print(str, 7);

    arr[0] = 'P';
    arr[1] = 'a';
    arr[2] = 's';
    arr[3] = 's';
    arr[4] = 'e';
    arr[5] = 'd';
    arr[6] = '\n';
    print(arr, 7);

    
    return 0;
}