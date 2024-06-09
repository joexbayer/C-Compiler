#include "./lib/test.c"
#include "./lib/std.c"
int main(){
    int arr[7];

    memset(arr, 9, 7);

    test(arr[0] == 9);
    test(arr[1] == 9);
    test(arr[2] == 9);
    test(arr[3] == 9);
    test(arr[4] == 9);
    test(arr[5] == 9);
    test(arr[6] == 9);
    

    return 0;
}