#include <cc.h>
#include <unistd.h> /* For open, read, close */
#include <fcntl.h> /* For open */

int cc_open(char *file, int flags) {
    return open(file, flags);
}

int cc_read(int fd, char *buffer, int size) {
    return read(fd, buffer, size);
}

int cc_close(int fd) {
    return close(fd);
}

void cc_write(int fd, char *buffer, int size) {
    write(fd, buffer, size);
}




