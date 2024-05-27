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
    int* b; 

    a = 1;
    b = &a;

    add(b, 2);  

    return a;
}