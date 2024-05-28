#include "cc.h"
#include "func.h"

struct function function_table[MAX_FUNCTIONS] = {0};
int function_id = 0;

int add_function(int id, char* name, int name_length, int* entry)
{
    if(function_id >= MAX_FUNCTIONS){
        printf("Maximum number of functions reached\n");
        exit(-1);
    }

    struct function *f = function_table + id;
    f->id = id;
    memcpy(f->name, name, name_length);
    
    /* Null-terminate the name */
    f->name[name_length] = '\0';

    f->entry = entry;

    return id;
}

struct function *find_function_name(char *name, int name_length)
{
    struct function *f = function_table;
    while (f->id) {
        if (strncmp(f->name, name, name_length) == 0) {
            return f;
        }
        f++;
    }
    return NULL;
}

struct function *find_function_id(int id){
    struct function *f = function_table;
    while (f) {
        if (f->id == id) {
            return f;
        }
        f++;
    }
    return NULL;
}
