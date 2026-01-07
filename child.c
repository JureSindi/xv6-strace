#include "types.h"
#include "stat.h"
#include "user.h"
#include "strace.h"

void forkP() {
    int index = fork();
    if(index == 0) {
        close(open("README", 0));
        exit();
    } 
    else {
        wait();
    }
}
int main() {
    strace(T_STRACE);
    forkP();
    strace(T_UNSTRACE);

    strace(T_STRACE | T_FORK);
    forkP();

    strace(T_UNSTRACE);
    exit();
}