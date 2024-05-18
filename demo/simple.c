#include <stdio.h>

void test_switch_if_case_break(int x) {

    // Test switch statement without break to fall through
    switch (x) {
        case 4:
            printf("x is four\n");
        case 5:
            printf("x is five\n");
        case 6:
            printf("x is six\n");
            break;
        default:
            printf("x is not 4, 5, or 6\n");
            break;
    }
}

int main() {
    // Test different values of x
    int x;
    x = 0;
    printf("Testing with x = 0\n");
    test_switch_if_case_break(6);
    
    printf("\nTesting with x = 4\n");
    test_switch_if_case_break(4);
    
    while (x < 10) {
        x++;
        printf("x = %d\n", x);

        switch (x)
        {
            case 1:
                printf("x is one\n");
                break;
            case 2:
                printf("x is two\n");
                break;
            case 3:
                printf("x is three\n");
                break;
            case 4:
                printf("x is four\n");
                break;
            case 5:
                printf("x is five\n");
                break;
            case 6:
                printf("x is six\n");
                break;
            case 7:
                printf("x is seven\n");
                break;
            case 8:
                printf("x is eight\n");
                break;
            case 9:
                printf("x is nine\n");
                break;
            case 10:
                printf("x is ten\n");
                break;
            default:
                printf("x is not 1-10\n");
                break;
        }
    }
    

    return 0;
}
