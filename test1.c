#include "types.h"
#include "stat.h"
#include "user.h"

int main() {
    int pid1, pid2;
    
    pid1 = fork();
    if(pid1 == 0) {
        nice(getpid(), 1); // Highest priority
        while(1) {
            printf(1, "High priority process  %d\n", getpid());
            sleep(10);
        }
    } 
    else {
        pid2 = fork();
        if(pid2 == 0) {
            nice(getpid(), 5); // Lowest priority
            while(1) { 
                printf(1, "Low priority process  %d\n", getpid());
                sleep(15);
            }
        } 
        else {
            wait();
            wait();
        }
    }
    exit();
}
