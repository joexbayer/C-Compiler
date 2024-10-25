struct object {
    int value;

    int get(struct object *object, int value) {
        object->value = value;
        return object->value;
    };

    int type;
};

int main(){
    struct object obj;
    obj.value = 10;

    obj.get(64);

    return obj.type;
}