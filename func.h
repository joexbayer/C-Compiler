#ifndef __FUNC_H
#define __FUNC_H

#define MAX_FUNCTIONS 256
struct function {
    int id;
    char name[25];
    int* entry;
};

extern int function_id;

#endif // !__FUNC_H