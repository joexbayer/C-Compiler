// File that tests pointers

int main(){
    int a;
    int b;
    int c;
    int *p;
    int *q;
    int *r;

    a = 10;
    b = 2;
    c = 0;

    p = &a;
    q = &b;
    r = &c;

    *r = *p + *q;
    *r = *p - *q;
    *r = *p * *q;
    *r = *p / *q;

    return *r;
}