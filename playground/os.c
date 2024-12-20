#include "./lib/std.c"

enum {
    BLACK = 0,
    BLUE = 1,
    GREEN = 2,
    CYAN = 3,
    RED = 4,
    MAGENTA = 5,
    BROWN = 6,
    LIGHT_GRAY = 7,
    DARK_GRAY = 8,
    LIGHT_BLUE = 9,
    LIGHT_GREEN = 10,
    LIGHT_CYAN = 11,
    LIGHT_RED = 12,
    LIGHT_MAGENTA = 13,
    LIGHT_BROWN = 14,
    WHITE = 15,
};

enum {
    VGA_WIDTH = 80,
    VGA_HEIGHT = 25,
};

void vgaput(int x, int y, char c, char color){
    char* vga;
    vga = 0xb8000;
    vga[2 * (y * VGA_WIDTH + x)] = c;
    vga[2 * (y * VGA_WIDTH + x) + 1] = color | BLUE << 4;
}

void vgastring(int x, int y, char* s, char color){
    int i;
    i = 0;
    while(s[i] != 0){
        vgaput(x + i, y, s[i], color);
        i = i + 1;
    }
}

void clear(){
    int x;
    int y;
    x = 0;
    y = 0;
    
    while(y < VGA_HEIGHT){
        while(x < VGA_WIDTH){
            vgaput(x, y, ' ', DARK_GRAY);
            x = x + 1;
        }
        x = 0;
        y = y + 1;
    }    
}

int vgaprint() {
    int i;
    char* text;
    void* vga;
    text = "Hello, World!\n";

    vga = &vgaprint;

    vgastring(0, 0, text, WHITE);
    text = "From my own C-Compiler!\n";
    vgastring(0, 1, text, WHITE);

    return i;
}

int main() {
    int j;
    char* vga;

    vga = 0xb8000;

    clear();

    vgaprint();

    //memset(vga, 0, 4000);

    j = 0;
    while(j < 1000){
        j = 0;
    }

    return j;
}