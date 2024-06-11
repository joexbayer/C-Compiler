#include "./lib/linux.c"

int memset(char* p, int val, int size){
    int i;
    i = 0;
    while(i < size){
        p[i] = val;
        i = i + 1;
    }
    return 1;
}

int memcpy(char* dest, char* src, int size){
    int i;
    i = 0;
    while(i < size){
        dest[i] = src[i];
        i = i + 1;
    }
    return 1;
}