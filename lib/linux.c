// Uses the interrupt 0x80 to call the kernel functions

enum {
    SYS_EXIT = 1,
    SYS_READ = 3,
    SYS_WRITE = 4,
    SYS_OPEN = 5,
    SYS_CLOSE = 6,
    SYS_MMAP = 192,
    SYS_MUNMAP = 91,
};

enum {
    O_RDONLY = 0,
    O_WRONLY = 1,
    O_RDWR = 2,
    O_CREAT = 64,
    O_TRUNC = 512,
    O_APPEND = 1024,
};

// Function to read from a file descriptor using read system call *
int alloc(int size) { 
    int ptr;
    ptr = __interrupt(0x80, SYS_MMAP, 0, size, 3, 34);
    return ptr;
}

// Function to read from a file descriptor using read system call *
int free(int* ptr, int size) {
    __interrupt(0x80, SYS_MUNMAP, ptr, size, 0, 0);
    return 0;
}

// Function to write to a file descriptor using write system call *
int print(char* str, int len) {
    __interrupt(0x80, SYS_WRITE, 1, str, len, 0);
    return 0;
}
// Function to exit the program using exit system call *
int exit(int status) {
    __interrupt(0x80, SYS_EXIT, status, 0, 0, 0);
    return 0;
}

// Function to read from a file descriptor using read system call *
int read(int fd, char* buf, int count) {
    return __interrupt(0x80, SYS_READ, fd, buf, count, 0);
}

// Read a line from the standard input
int scanline(char* buf, int count) {
    char* tmp;
    int i;
    i = 0;
    while (i < count) {
        tmp = buf + i;
        read(0, tmp, 1);
        if (*tmp == '\n') {
            *(tmp + 1) = 0;
            return i + 1;
        }
        i = i + 1;
    }
    return i;
}

// Function to open a file using open system call *
int open(char* filename, int flags, int mode) {
    int fd;
    fd = __interrupt(0x80, SYS_OPEN, filename, flags, mode, 0);
    return fd;
}

// Function to close a file descriptor using close system call *
int close(int fd) {
    __interrupt(0x80, SYS_CLOSE, fd, 0, 0, 0);
    return 0;
}
