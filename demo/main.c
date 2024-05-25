#include "./demo/math.c"

struct math_ops {
    int x;
    int y;
    int op; // Operator: '+', '-', '*', '/'
};


int perform(struct math_ops* mo) {
    int result;

    if (mo->op == '+') {
        result = add(mo->x, mo->y);
    } else if (mo->op == '-') {
        result = sub(mo->x, mo->y);
    } else if (mo->op == '*') {
        result = mul(mo->x, mo->y);
    } else if (mo->op == '/') {
        result = div(mo->x, mo->y);
    } else {
        result = 0;
    }

    return result;
}

int main(int argc, char** argv) {
    struct math_ops mo1;
    int result;
    int i;
    i = 0;

    mo1.x = 10;
    mo1.y = 5;

    printf("Value of x: %d\n", mo1.x);
    printf("Value of y: %d\n", mo1.y);
    
    mo1.op = '*';
    result = perform(&mo1); 
    printf("Final result: %d\n", result);

    mo1.op = '/';
    mo1.x = 30;
    mo1.y = 5;
    result = perform(&mo1);
    printf("Final result: %d\n", result);

    while (i < 10){
        i = i + 1;
        printf("Final value of i: %d\n", i);
    }


    result = perform(&mo1);

    printf("Final result: %d\n", result);

    return 0;
}
