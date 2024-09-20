#ifndef __IO_H__
#define __IO_H__

#include <stdio.h>

int cc_open(char *file, int flags);
int cc_read(int fd, char *buffer, int size);
int cc_close(int fd);
void cc_write(int fd, char *buffer, int size);

#endif