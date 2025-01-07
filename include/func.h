#ifndef __FUNC_H
#define __FUNC_H

#define MAX_FUNCTIONS 256
struct function {
    int id;
    char name[64];
    int* entry;
};

extern int function_id;
int add_function(int id, char* name, int name_length, int* entry);  
struct function *find_function_name(char *name, int name_length);
struct function *find_function_id(int id);  

#endif // !__FUNC_H
