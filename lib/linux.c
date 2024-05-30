// Uses the interrupt 0x80 to call the kernel functions

int alloc(int size) { 
    int ptr;
    // 192 is mmap
    ptr = __interrupt(0x80, 192, 0, size, 3, 34);
    return ptr;
}

int free(int* ptr, int size) {
    // 91 is munmap
    __interrupt(0x80, 91, ptr, size, 0, 0);
    return 0;
}

int print(char* str, int len) {
    // 4 is write
    __interrupt(0x80, 4, 1, str, len, 0);
    return 0;
}
