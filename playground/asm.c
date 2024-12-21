#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>

typedef enum {
    REG_EAX = 0,
    REG_ECX = 1,
    REG_EDX = 2,
    REG_EBX = 3,
    REG_ESP = 4,
    REG_EBP = 5,
    REG_ESI = 6,
    REG_EDI = 7
} x86_reg_t;

typedef enum {
    ERR = 0,
    MOV = 1,
    ADD = 2,
    SUB = 3,
    MUL = 4,
    DIV = 5,
    PUSH = 6,
    POP = 7,
    RET = 8,
    CALL = 9,
    JMP = 10,
    JE = 11,
    JNE = 12,
    CMP = 13,
    LEA = 14,
    INT = 15
} x86_opcode_t;

/* A simple instruction set */
typedef struct {
    x86_opcode_t op;
    const char* name;
    uint8_t opcode;
} instruction_t;

instruction_t instruction_set[] = {
    { MOV, "mov", 0x89 },
    { ADD, "add", 0x01 },
    { SUB, "sub", 0x29 },
    { MUL, "mul", 0xf7 },
    { DIV, "div", 0xf7 },
    { PUSH, "push", 0x50 },
    { POP, "pop", 0x58 },
    { RET, "ret", 0xc3 },
    { CALL, "call", 0xe8 },
    { JMP, "jmp", 0xe9 },
    { JE, "je", 0x74 },
    { JNE, "jne", 0x75 },
    { CMP, "cmp", 0x39 },
    { LEA, "lea", 0x8d },
    { INT, "int", 0xcd },
    { 0, NULL, 0 }
};

// Helper to find instruction opcode
instruction_t find_opcode(const char* name) {
    for (int i = 0; instruction_set[i].name; i++) {
        if (strcmp(instruction_set[i].name, name) == 0) {
            return instruction_set[i];
        }
    }
    return (instruction_t){0};
}

// Skip leading whitespace
char* skip_whitespace(char* str) {
    while (*str && isspace((unsigned char)*str)) {
        str++;
    }
    return str;
}

char* get_token(char** input) {
    char* start = skip_whitespace(*input);
    if (*start == '\0') {
        return NULL; // End of string
    }

    char* end = start;
    while (*end && !isspace((unsigned char)*end) && *end != ',' && *end != '\n') {
        end++;
    }

    // Extract the token
    size_t length = end - start;
    char* token = (char*)malloc(length + 1);
    if (!token) {
        printf("Memory allocation failed.\n");
        exit(EXIT_FAILURE);
    }
    strncpy(token, start, length);
    token[length] = '\0';

    // Advance the input pointer
    *input = skip_whitespace(end);

    return token;
}

void append_opcode(uint8_t** opcodes, int* capacity, int* size, uint8_t value) {
    if (*size >= *capacity) {
        *capacity *= 2;
        *opcodes = realloc(*opcodes, *capacity);
        if (!*opcodes) {
            printf("Memory allocation failed.\n");
            exit(EXIT_FAILURE);
        }
    }
    (*opcodes)[(*size)++] = value;
}

int get_register_code(const char* reg) {
    if (strcmp(reg, "eax") == 0) return REG_EAX;
    if (strcmp(reg, "ebx") == 0) return REG_EBX;
    if (strcmp(reg, "ecx") == 0) return REG_ECX;
    if (strcmp(reg, "edx") == 0) return REG_EDX;
    return -1; // Unsupported register
}

int compile_asm(const char* asm_code, uint8_t** opcodes, int* opcodes_count) {
    char* current_position = strdup(asm_code); // Duplicate the input string
    if (!current_position) {
        printf("Memory allocation failed for input copy.\n");
        free(*opcodes);
        return -1;
    }

    char* original_position = current_position; // Save for freeing later

    while (1) {
        char* token = get_token(&current_position);
        if (!token) {
            break;
        }

        instruction_t instr = find_opcode(token);
        if (instr.op == ERR) {
            printf("Unknown instruction: %s\n", token);
            free(token);
            free(*opcodes);
            free(original_position); // Free the strdup memory
            return -1;
        }

        opcodes[]

        switch (instr.op) {
            case MOV:
            case ADD:
            case SUB:
            case CMP: {
                char* dest = get_token(&current_position);
                if (*current_position == ',') {
                    current_position++;
                }
                char* src = get_token(&current_position);

                if (!dest || !src) {
                    printf("Invalid operands for %s\n", instr.name);
                    free(token);
                    free(dest);
                    free(src);
                    free(*opcodes);
                    free(original_position);
                    return -1;
                }

                int reg_code = get_register_code(dest);
                if (reg_code == -1) {
                    printf("Unsupported destination register: %s\n", dest);
                    free(token);
                    free(dest);
                    free(src);
                    free(*opcodes);
                    free(original_position);
                    return -1;
                }

                append_opcode(opcodes, &capacity, opcodes_count, 0xC0 | reg_code);

                int src_reg_code = get_register_code(src);
                if (src_reg_code != -1) {
                    append_opcode(opcodes, &capacity, opcodes_count, src_reg_code);
                } else if (src[0] == '0' && (src[1] == 'x' || src[1] == 'X')) {
                    uint32_t imm = strtoul(src, NULL, 16);
                    memcpy((*opcodes) + *opcodes_count, &imm, sizeof(uint32_t));
                    *opcodes_count += 4;
                } else {
                    printf("Unsupported source operand: %s\n", src);
                    free(token);
                    free(dest);
                    free(src);
                    free(*opcodes);
                    free(original_position);
                    return -1;
                }

                free(dest);
                free(src);
                break;
            }

            case PUSH:
            case POP: {
                char* reg = get_token(&current_position);
                if (!reg) {
                    printf("Invalid operand for %s\n", instr.name);
                    free(token);
                    free(*opcodes);
                    free(original_position);
                    return -1;
                }

                int reg_code = get_register_code(reg);
                if (reg_code == -1) {
                    printf("Unsupported register: %s\n", reg);
                    free(token);
                    free(reg);
                    free(*opcodes);
                    free(original_position);
                    return -1;
                }

                append_opcode(opcodes, &capacity, opcodes_count, instr.opcode | reg_code);
                free(reg);
                break;
            }

            case JMP:
            case CALL: {
                char* target = get_token(&current_position);
                if (!target) {
                    printf("Invalid operand for %s\n", instr.name);
                    free(token);
                    free(*opcodes);
                    free(original_position);
                    return -1;
                }

                uint32_t address = strtoul(target, NULL, 16);
                memcpy((*opcodes) + *opcodes_count, &address, sizeof(uint32_t));
                *opcodes_count += 4;

                free(target);
                break;
            }

            case RET:
                // No operands
                break;

            default:
                printf("Unsupported instruction: %s\n", instr.name);
                free(token);
                free(*opcodes);
                free(original_position);
                return -1;
        }

        free(token);

        if (*current_position == '\n') {
            current_position++;
        }
    }

    free(original_position); // Free strdup memory
    return 0;
}

int main() {
    const char* asm_code = "mov eax, 0x1234\n"
                           "add ebx, 0x5678\n"
                           "sub eax, ebx\n"
                           "cmp eax, ebx\n"
                           "push eax\n"
                           "pop ebx\n"
                           "call 0x00400000\n"
                           "jmp 0x00400010\n"
                           "ret\n";
    uint8_t* opcodes;
    int opcodes_count;

    if (compile_asm(asm_code, &opcodes, &opcodes_count) < 0) {
        printf("Failed to compile asm\n");
        return -1;
    }

    for (int i = 0; i < opcodes_count; i++) {
        printf("%02x ", opcodes[i]);
    }
    printf("\n");

    free(opcodes);
    return 0;
}
