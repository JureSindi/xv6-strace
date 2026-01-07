#include "types.h"
#include "stat.h"
#include "user.h"

void primes() {
    int i = 2;
    while (1) {
        int prime = 1;
        int j;

        for (j = 2; j * j <= i; j++) {
            if (i % j == 0) {
                prime = 0;
                break;
            }
        }

        if (prime) {
            printf(1, "Process  %d and Prime  %d\n", getpid(), i);
        }

        i++;
        if (i > 1000000) {
            i = 2;
        }
    }
}

int main() {
    int pid1, pid2, pid3;

    pid1 = fork();
    if (pid1 == 0) {
        nice(getpid(), 1);  // Highest priority
        primes();
        exit();
    }

    pid2 = fork();
    if (pid2 == 0) {
        nice(getpid(), 3);  // Medium priority
        primes();
        exit();
    }

    pid3 = fork();
    if (pid3 == 0) {
        nice(getpid(), 5);  // Lowest priority
        primes();
        exit();
    }

    sleep(300);

    // Killing the children processes after time limit
    kill(pid1);
    kill(pid2);
    kill(pid3);

    // Waiting for all children processes to terminate
    wait();
    wait();
    wait();

    exit();
}
