

struct math_ops {
    int x;
    int y;
    int op; // Operator: '+', '-', '*', '/'
};

int h(){
    printf("Hello\n");
}

int add(int a, int b) {
    printf("Adding %d and %d\n", a, b);
    return a + b;
}

int perform() {
    int result;

    result = 10;
    
    printf("Result of addition: %d\n", result);
    return result;
}

int main(int argc, char** argv) {
    struct math_ops mo1;
    int result;
    int i;
    i = 0;

    mo1.x = 10;
    mo1.y = 5;
    
    mo1.op = '*';
    result = perform();

    mo1.op = '/';
    mo1.x = 30;
    mo1.y = 5;
    result = perform();

    while (i < 10){
        i = i + 1;
        printf("Final value of i: %d\n", i);
    }


    result = perform();

    printf("Final result: %d\n", result);

    return 0;
}
