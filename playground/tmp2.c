struct IDTEntry {
    char offset_low1;    // Lower 8 bits of the handler function address
    char offset_low2;    // Next 8 bits of the handler function address
    char selector1;      // Lower 8 bits of the kernel segment selector
    char selector2;      // Upper 8 bits of the kernel segment selector
    char zero;           // Reserved, set to 0
    char type_attr;      // Type and attributes (e.g., 0x8E for 32-bit interrupt gate)
    char offset_high1;   // Lower 8 bits of the upper 16 bits of the handler function address
    char offset_high2;   // Upper 8 bits of the upper 16 bits of the handler function address
};

struct IDTEntry idt[256];

void idt_set(struct IDTEntry* entry, void* handler, int selector, char type_attr){
    entry->offset_low1 = (int)handler & 0xFF;
    entry->offset_low2 = ((int)handler >> 8) & 0xFF;
    entry->selector1 = selector & 0xFF;
    entry->selector2 = (selector >> 8) & 0xFF;
    entry->zero = 0;
    entry->type_attr = type_attr;
    entry->offset_high1 = ((int)handler >> 16) & 0xFF;
    entry->offset_high2 = ((int)handler >> 24) & 0xFF;
}

int test(){
    return 0;
}

int main(){
    struct IDTEntry* entry;
    int i;
    entry = &idt[0];
    i = &test;

    entry->offset_low1 = 0x00;

    idt_set(entry, i, 0x08, 0x8E);

    return 0;
}