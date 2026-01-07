#include "types.h"
#include "stat.h"
#include "user.h"

void fibonacci() {
    int a = 0, b = 1, c;

    while (1) {
        c = a + b;
        a = b;
        b = c;

        printf(1, "Process  %d and Fibonacci  %d\n", getpid(), c);

        if (c > 1000000) {
            a = 0;
            b = 1;
        }

        sleep(1);
    }
}

int main() {
    int pid1 = fork();
    if (pid1 == 0) {
        nice(getpid(), 1);  // Highest priority
        fibonacci();
        exit();
    }

    int pid2 = fork();
    if (pid2 == 0) {
        nice(getpid(), 3);  // Medium priority
        fibonacci();
        exit();
    }

    int pid3 = fork();
    if (pid3 == 0) {
        nice(getpid(), 5);  // Lowest priority
        fibonacci();
        exit();
    }

    sleep(300);

    // Killing children processes after time limit
    kill(pid1);
    kill(pid2);
    kill(pid3);

    // Waiting for all children processes to terminate
    wait();
    wait();
    wait();

    exit();
}