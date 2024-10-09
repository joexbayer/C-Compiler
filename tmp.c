struct object {
    int type;
    int value;

    int get() {
        return 69;
    };
};

int get(struct object *obj){
    return obj->value;
}

int main(){
    return object_get();
}