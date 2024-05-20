#ifndef DF579CF4_EFA3_4966_AEE7_98CCF666A06B
#define DF579CF4_EFA3_4966_AEE7_98CCF666A06B

#include <unistd.h> /* For open, read, close */
#include <stdio.h>
#include <stdlib.h>
#include <memory.h> /* For memcmp & strcmp */
#include <fcntl.h> /* For open */
#include <stdarg.h> /* For va_list */
#include <time.h>


int* read_bytecode(const char *filename, size_t *code_size, char **data, size_t *data_size, int *main_pc);
void write_bytecode(const char *filename, int *code, size_t code_size, char *data, size_t data_size, int *main_pc);

#endif /* DF579CF4_EFA3_4966_AEE7_98CCF666A06B */
