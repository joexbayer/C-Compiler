
int vgaprint() {
    int i;
    char* vga;
    char* text;
    text = "Hello, World!\n";
    vga = 0xb8000;
    
    while(i < 13){
       
        vga[0] = text[i];
        vga[1] = 0x07;

        vga = vga + 2;

        i = i + 1;
    }

    return i;
}

int main() {
    int j;
    vgaprint();

    j = 0;
    while(j < 1000){
        j = 0;
    }

    return j;
}