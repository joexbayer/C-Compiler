#include "./demo/math.c"

// Definition of struct math_operations
struct math_operations {
    int x;
    int y;
    int op; // Operator: '+', '-', '*', '/'
};

struct math_operations mo;

// Function to test passing struct pointers and performing math operations
int perform_operations(struct math_operations* mo) {
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

// Main function 
int main() {
    int result;
    struct math_operations mo1; // Variable of type struct math_operations

    mo1.x = 10;
    mo1.y = 5;

    mo.op = '+';
    mo.x = 20;
    mo.y = 10;

    printf("Initial values: x = %d, y = %d\n", mo.x, mo.y);
    if (mo.x == 20) {
        printf("mo.x is 20\n");
    } else {
        printf("mo.x is not 20\n");
    }
    
    mo1.op = '*';
    result = perform_operations(&mo1);

    mo1.op = '/';
    mo1.x = 30;
    mo1.y = 5;
    result = perform_operations(&mo1);

    printf("Final operation: %c\n", mo.op);
    result = perform_operations(&mo);

    printf("Hello, World! Final result: %d\n", result);

    return 0;
}
