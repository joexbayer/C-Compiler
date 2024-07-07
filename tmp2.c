#include "./lib/test.c"

int val(int x, char c){
    
    test(c == 'H');

    return 0;
}

int main(){
    int i;
    char* text;
    char c; // currently needs to be int
    text = "Hello, World!\n";
    c = 'H';

    c = text[0];

    val(0, c);

    return 0;
}