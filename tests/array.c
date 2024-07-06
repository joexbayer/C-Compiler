#include "./lib/test.c"
int main(){
    char arr[3];
    char* arr2;
    arr2 = "abc";
    
    arr[0] = 1;
    arr[1] = 2;
    arr[2] = 3;

    test(arr[0] == 1);
    test(arr[1] == 2);
    test(arr[2] == 3);

    arr2[0] = 1;
    arr2[1] = 2;
    arr2[2] = 3;

    test(arr2[0] == 1);
    test(arr2[1] == 2);
    test(arr2[2] == 3);


    return 0;
}