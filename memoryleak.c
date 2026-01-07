#include "types.h"
#include "stat.h"
#include "user.h"
#include "strace.h"

int main() {
    // Here this is staring the stracing system calls
    strace(T_STRACE);

    int *index;
    int indexS = sizeof(int) * 100000000;

    while(1) {
        index = malloc(indexS);
        if(index == 0) {
            printf(1, "Memory allocation failed.\n");
            break;
        }
    }
    exit();
}
