#include "./demo/math.c"

struct math_ops {
    int x;
    int y;
    int op; // Operator: '+', '-', '*', '/'
};
struct math_ops mo;

int perform(struct math_ops* mo) {
    int result;
    switch (mo->op) {
        case '+':
            result = add(mo->x, mo->y);
            break;
        case '-':
            result = sub(mo->x, mo->y);
            break;
        case '*':
           result = mul(mo->x, mo->y);
            break;
        case '/':
            result = div(mo->x, mo->y);
            break;
        default:
            printf("Invalid operation\n");
            return -1;
    }
    
    printf("Operation %c on %d and %d gives result: %d\n", mo->op, mo->x, mo->y, result);
    return result;
}

int main(int argc, char** argv) {
    struct math_ops mo1;
    int result;
    int i;

    mo1.x = 10;
    mo1.y = 5;
    
    mo1.op = '*';
    result = perform(&mo1);

    mo1.op = '/';
    mo1.x = 30;
    mo1.y = 5;
    result = perform(&mo1);

    while (i < 10){
        i++;
        mo.x = i;
    }

    mo.op = '+';
    mo.y = 10;

    printf("Initial values: x = %d, y = %d\n", mo.x, mo.y);
    if (mo.x == 20) {
        printf("mo.x is 20\n");
    } else {
        printf("mo.x is not 20\n");
    }
    printf("Final operation: %c\n", mo.op);
    result = perform(&mo);

    printf("Final result: %d\n", result);

    return 0;
}
