#include "./lib/test.c"

struct test {
    int a;
    int b;
};

struct object {
    int value;

    int get(struct object *object, int value) {
        object->value = value;
        return 0;
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

int get2(struct object *object, int value) {
    int type;
    type = value;
    object->value = type;
    return 0;
};


int main() {
    struct object obj;
    obj.value = 10;

    test(obj.value == 10);
    obj.get(64);

    test(obj.value == 64);

    get2(&obj, 64);
    
    test(obj.value == 64);

    obj.increment(5);

    test(obj.value == 69);
    
    obj.reset();

    test(obj.value == 0);

    obj.type = 1;

    test(obj.type == 1);

    return obj.value;
}
