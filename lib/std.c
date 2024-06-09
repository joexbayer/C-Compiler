int memset(int* p, int val, int size){
    int i;
    i = 0;
    while(i < size){
        p[i] = val;
        i = i + 1;
    }
    return 1;
}