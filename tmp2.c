struct object {
    int value;
    int get;
    int type;
};

int get(struct object *object, int value) {
    object->value = value;
    return object->value;
};

int main(){
    struct object obj;
    obj.value = 10;

    get(&obj, 64);

    return obj.type;
}