// Uses the interrupt 0x80 to call the kernel functions

enum {
    SYS_EXIT = 1,
    SYS_WRITE = 4,
    SYS_MMAP = 192,
    SYS_MUNMAP = 91
};

int alloc(int size) { 
    int ptr;
    // 192 is mmap
    ptr = __interrupt(0x80, SYS_MMAP, 0, size, 3, 34);
    return ptr;
}

int free(int* ptr, int size) {
    // 91 is munmap
    __interrupt(0x80, SYS_MUNMAP, ptr, size, 0, 0);
    return 0;
}

int print(char* str, int len) {
    // 4 is write
    __interrupt(0x80, SYS_WRITE, 1, str, len, 0);
    return 0;
}

int exit(int status) {
    // 1 is exit
    __interrupt(0x80, SYS_EXIT, status, 0, 0, 0);
    return 0;
}
