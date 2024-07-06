#include "./lib/test.c"

int vgaput(char c){
    char n;
    // Here im overwriting the value of n with a INT, corrupting the stack. Somehow...
    n = c;

    print(&c, 1);

    return 0;
}

int main(){
    int i;
    char c; // currently needs to be int
    char* vga;
    char* text;
    text = "Hello, World!\n";
    
    //vgaput('e');
    
    i = 0;
    c = 0;
    while (i < 13) {
        vgaput(text[i]);
        i = i + 1;
    }

    return 0;
}