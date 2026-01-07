#include <stdio.h>
#include <unistd.h>

int main() {
    int i, n = 0;
    char buf[512];
    n = read(0, buf, 512);  // Read from stdin
    for (i = 0; i < 10 && i < n; i++) {
        printf("%c\n", buf[i]);
    }
    return 0;
}