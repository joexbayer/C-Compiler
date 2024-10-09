#include <cc.h>

int cc_open(char *file, int flags) {
    return open(file, flags);
}

int cc_read(int fd, char *buffer, int size) {
    return read(fd, buffer, size);
}

int cc_close(int fd) {
#ifdef NATIVE
    return close(fd);
#else
    return fclose(fd);
#endif
}

void cc_write(int fd, char *buffer, int size) {
    write(fd, buffer, size);
}

#ifdef NATIVE
void *zmalloc(int size) {
    return malloc(size);
}
#endif



