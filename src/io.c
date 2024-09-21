#include <cc.h>

int cc_open(char *file, int flags) {
    return open(file, flags);
}

int cc_read(int fd, char *buffer, int size) {
    return read(fd, buffer, size);
}

int cc_close(int fd) {
    return fclose(fd);
}

void cc_write(int fd, char *buffer, int size) {
    write(fd, buffer, size);
}




