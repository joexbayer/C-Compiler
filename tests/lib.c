#include "./lib/test.c"
#include "./lib/std.c"

int scantest(){
    int len;
    char* a;
    a = alloc(100);

    print("Enter a string: ", 16);
    len = scanline(a, 100);

    print("You entered: ", 13);
    print(a, len);

    return 0;
}

int main(){
    char arr[7];
    char arr2[7];

    memset(arr, 9, 7);

    test(arr[0] == 9);
    test(arr[1] == 9);
    test(arr[2] == 9);
    test(arr[3] == 9);
    test(arr[4] == 9);
    test(arr[5] == 9);
    test(arr[6] == 9);

    memcpy(arr2, arr, 7);

    test(arr2[0] == 9);
    test(arr2[1] == 9);
    test(arr2[2] == 9);
    test(arr2[3] == 9);
    test(arr2[4] == 9);
    test(arr2[5] == 9);
    test(arr2[6] == 9);

    scantest();

    return 0;
}