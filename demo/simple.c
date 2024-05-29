int alloc(int size) {
    int ptr;
    ptr = __interrupt(0x80, 192, 0, size, 3, 34);
    return ptr;
}

int free(int* ptr, int size) {
    __interrupt(0x80, 91, ptr, size, 0, 0);
    return 0;
}

int print(char* str, int len) {
    __interrupt(0x80, 4, 1, str, len, 0);
    return 0;
}

int main() {
    char* a;

    a = alloc(4096);

    a[0] = 'H';
    a[1] = 'e';
    a[2] = 'l';
    a[3] = 'l';
    a[4] = 'o';
    a[5] = 10;
    a[6] = 0;

    print(a, 7);

    free(a, 4096);

    return 0;
}