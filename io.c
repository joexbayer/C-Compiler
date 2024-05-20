#include "cc.h"

int* read_bytecode(const char *filename, size_t *code_size, char **data, size_t *data_size, int *main_pc) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        printf("Unable to open file for reading: %s\n", filename);
        exit(-1);
    }

    /* Read entry code first int */
    fread(main_pc, sizeof(int*), 1, file);
    
 /* Read code size and code */
    fread(code_size, sizeof(size_t), 1, file);
    int *code = (int *)malloc(*code_size * sizeof(int));
    if (!code) {
        printf("Unable to allocate memory for machine code\n");
        exit(-1);
    }
    for (size_t i = 0; i < *code_size; i++){
        /* Read code as shorts */
        short s;
        fread(&s, sizeof(short), 1, file);
        code[i] = s;
    }

    /* Read data size and data */
    fread(data_size, sizeof(size_t), 1, file);
    *data = (char *)malloc(*data_size * sizeof(char));
    if (!*data) {
        printf("Unable to allocate memory for data segment\n");
        exit(-1);
    }
    fread(*data, sizeof(char), *data_size, file);

    fclose(file);
    dbgprintf("Bytecode read from %s (%d bytes)\n", filename, (int)(sizeof(int*) + *code_size * sizeof(int) + *data_size * sizeof(char)));
    return code;
}

void write_bytecode(const char *filename, int *code, size_t code_size, char *data, size_t data_size, int *main_pc) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        printf("Unable to open file for writing: %s\n", filename);
        exit(-1);
    }
    
    /* Write entry point */
    fwrite(main_pc, sizeof(int), 1, file);
      
    /* Write code section as shorts to save space. */
    fwrite(&code_size, sizeof(size_t), 1, file);
    for (size_t i = 0; i < code_size; i++){
        /* Write code as shorts */
        short s = code[i];
        fwrite(&s, sizeof(short), 1, file);
    }
    
    /* Write data section */
    fwrite(&data_size, sizeof(size_t), 1, file);
    fwrite(data, sizeof(char), data_size, file);
    fclose(file);

    dbgprintf("Bytecode written to %s (%d bytes)\n", filename, (int)(sizeof(int*) + code_size * sizeof(short) + data_size * sizeof(char)));
}






