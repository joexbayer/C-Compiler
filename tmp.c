#include "./lib/test.c"

void vgaput(int x, int y, char c, char color){
    char* vga;
    vga = 0xb8000;
   
    print(&c, 1);
}
int main(){
    int i;
    char c; // currently needs to be int
    char* vga;
    char* text;
    text = "Hello, World!\n";
    
    //vgaput('e');
    
    i = 0;
    //c = 0;
    while (i < 13) {
        vgaput(0, 0, text[i], 0);
        i = i + 1;
    }

    return 0;
}