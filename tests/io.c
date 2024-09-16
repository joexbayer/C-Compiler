#include "./lib/linux.c"

int main(){
    int fd;
    int n;
    char* a;
    char buffer[100];
    a = "text.txt";

    fd = open(a, O_RDONLY, 0);
    if (fd < 0){
        print("Failed to open file\n", 20);
        return 1;
    }

    n = read(fd, buffer, 100);
    if (n < 0){
        print("Failed to read file\n", 20);
        return 1;
    }

    print(buffer, n);
    close(fd);

    return 0;
}