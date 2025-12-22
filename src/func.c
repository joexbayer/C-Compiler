#include <cc.h>
#include <func.h>

struct function function_table[MAX_FUNCTIONS] = {0};
int function_id = 0;

int add_function(int id, char* name, int name_length, int* entry)
{
    if (id < 0 || id >= MAX_FUNCTIONS) {
        printf("Function id out of range\n");
        exit(-1);
    }
    if (name_length <= 0 || name_length >= (int)sizeof(function_table[id].name)) {
        printf("Function name too long\n");
        exit(-1);
    }
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

struct function *find_function_name(char *name, int name_length){
    if (name_length <= 0 || name_length >= (int)sizeof(function_table[0].name)) {
        return NULL;
    }
    for (int i = 0; i < function_id && i < MAX_FUNCTIONS; i++) {
        struct function *f = &function_table[i];
        if (f->name[0] != '\0' && f->name[name_length] == '\0' &&
            strncmp(f->name, name, name_length) == 0) {
            return f;
        }
    }
    return NULL;
}

struct function *find_function_id(int id){
    if (id < 0 || id >= MAX_FUNCTIONS) {
        return NULL;
    }
    for (int i = 0; i < function_id && i < MAX_FUNCTIONS; i++) {
        struct function *f = &function_table[i];
        if (f->id == id) {
            return f;
        }
    }
    return NULL;
}
