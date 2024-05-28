struct math {
    int a;
    int b;
    int c;
};

int add(int* x, int a) {
    *x = a;
    return 0;
}

int main() {
    int a;

    a = 1;

    while (a < 5) {
        
        if(a == 2) {
            a = a + 5;
        } else {
            a = a + 1;
        }

    }


    return a;
}