#include "lib/linux.c"

struct test {
    int a;
    int b;
};

struct object {
    int value;

    int get(struct object *object, int value) {
        int type;
        type = value;
        object->value = type;
        return object->value;
    };

    int reset(struct object *object) {
        object->value = 0;
        return object->value;
    };

    int increment(struct object *object, int incrementValue) {
        object->value = object->value + incrementValue;
        return object->value;
    };

    int type;
};

int main() {
    struct object obj;
    obj.value = 10;

    obj.get(64);
    
    obj.increment(5);
    
    obj.reset();

    obj.type = 1;

    return obj.value;
}
