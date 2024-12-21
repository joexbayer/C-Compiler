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
    vga[2 * (y * VGA_WIDTH + x)+1] = color | BLUE << 4;
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
            vgaput(x, y, ' ', WHITE);
            x = x + 1;
        }
        x = 0;
        y = y + 1;
    }    
}

char scancode_map[100];
void populate_scancode_map() {
    scancode_map[0x00] = 0;
    scancode_map[0x01] = 27;
    scancode_map[0x02] = '1';
    scancode_map[0x03] = '2';
    scancode_map[0x04] = '3';
    scancode_map[0x05] = '4';
    scancode_map[0x06] = '5';
    scancode_map[0x07] = '6';
    scancode_map[0x08] = '7';
    scancode_map[0x09] = '8';
    scancode_map[0x0A] = '9';
    scancode_map[0x0B] = '0';
    scancode_map[0x0C] = '-';
    scancode_map[0x0D] = '=';
    scancode_map[0x0E] = '\b';
    scancode_map[0x0F] = '\t';
    scancode_map[0x10] = 'q';
    scancode_map[0x11] = 'w';
    scancode_map[0x12] = 'e';
    scancode_map[0x13] = 'r';
    scancode_map[0x14] = 't';
    scancode_map[0x15] = 'y';
    scancode_map[0x16] = 'u';
    scancode_map[0x17] = 'i';
    scancode_map[0x18] = 'o';
    scancode_map[0x19] = 'p';
    scancode_map[0x1A] = '[';
    scancode_map[0x1B] = ']';
    scancode_map[0x1C] = '\n';
    scancode_map[0x1D] = 0;
    scancode_map[0x1E] = 'a';
    scancode_map[0x1F] = 's';
    scancode_map[0x20] = 'd';
    scancode_map[0x21] = 'f';
    scancode_map[0x22] = 'g';
    scancode_map[0x23] = 'h';
    scancode_map[0x24] = 'j';
    scancode_map[0x25] = 'k';
    scancode_map[0x26] = 'l';
    scancode_map[0x27] = ';';
    scancode_map[0x28] = '\'';
    scancode_map[0x29] = '`';
    scancode_map[0x2A] = 0;
    scancode_map[0x2B] = '\\';
    scancode_map[0x2C] = 'z';
    scancode_map[0x2D] = 'x';
    scancode_map[0x2E] = 'c';
    scancode_map[0x2F] = 'v';
    scancode_map[0x30] = 'b';
    scancode_map[0x31] = 'n';
    scancode_map[0x32] = 'm';
    scancode_map[0x33] = ',';
    scancode_map[0x34] = '.';
    scancode_map[0x35] = '/';
    scancode_map[0x36] = 0;
    scancode_map[0x37] = '*';
    scancode_map[0x38] = 0;
    scancode_map[0x39] = ' ';
    scancode_map[0x3A] = 0;
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
    int c;
    int key;
    int i;
    char* vga;

    i  = 0;

    vga = 0xb8000;

    populate_scancode_map();

    clear();

    vgaprint();

    __outportb(0x20, 0x20);
    while(1){
        if(__inportb(0x64) & 0x01 == 1){
            c = __inportb(0x60);
          
            if(!(c & 0x80)){
                c = c & 0x7F;  
                key = scancode_map[c];
                vgaput(i, 0, key, WHITE);
                i = i + 1;
            }            
            __outportb(0x20, 0x20);
        }
    }

    return j;
}