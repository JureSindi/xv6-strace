#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

#define LOCK_ID 2

// etest3 satisfies etest2, it adds priority inheritance 

// Timestamp
int get_time() {
    return uptime();
}

// Log success/failure w/ timestamp
void log_result(int success, const char *message) {
    if (success) {
        printf(1, "[Time  %d] SUCCESS:  %s\n", get_time(), message);
    } 
    else {
        printf(1, "[Time  %d] FAILURE:  %s\n", get_time(), message);
    }
}

void low_priority() {
    nice(getpid(), 5);
    printf(1, "[Time  %d] Low-priority process  %d  started with priority  %d\n", get_time(), getpid(), 5);

    printf(1, "[Time  %d] Low-priority process  %d  attempting to acquire lock %d (Should acquire lock)\n", get_time(), getpid(), LOCK_ID);
    if (lock(LOCK_ID) == 0) {
        log_result(1, "Low-priority process acquired lock");

        int i;
        for (i = 0; i < 5; i++) {
            printf(1, "[Time  %d] Low-priority process  %d  is working (%d  seconds)\n", get_time(), getpid(), i + 1);
            sleep(100);
        }

        printf(1, "[Time  %d] Low-priority process  %d  releasing lock  %d (Should release lock)\n", get_time(), getpid(), LOCK_ID);
        if (release(LOCK_ID) == 0) {
            log_result(1, "Low-priority process released lock");
            printf(1, "[Time  %d] Low-priority process  %d  priority restored to  %d  after releasing lock\n", get_time(), getpid(), 5);
        } 
        else {
            log_result(0, "Low-priority process failed to release lock");
        }
    } 
    else {
        log_result(0, "Low-priority process failed to acquire lock");
    }
    exit();
}

void high_priority() {
    sleep(300);

    nice(getpid(), 1);
    printf(1, "[Time  %d] High-priority process  %d  started with priority  %d\n", get_time(), getpid(), 1);

    printf(1, "[Time  %d] High-priority process  %d  attempting to acquire lock %d (Should wait if lock is held)\n", get_time(), getpid(), LOCK_ID);
    if (lock(LOCK_ID) == 0) {
        log_result(1, "High-priority process acquired lock after waiting");

        printf(1, "[Time  %d] High-priority process  %d  releasing lock  %d (Should release lock)\n", get_time(), getpid(), LOCK_ID);
        if (release(LOCK_ID) == 0) {
            log_result(1, "High-priority process released lock");
        } 
        else {
            log_result(0, "High-priority process failed to release lock");
        }
    } 
    else {
        log_result(0, "High-priority process failed to acquire lock");
    }
    exit();
}

int main() {
    printf(1, "[TEST START] Test 3: Priority Inversion (Priority Inheritance)\n");

    int pid1 = fork();
    if (pid1 == 0) {
        low_priority();
    }

    int pid2 = fork();
    if (pid2 == 0) {
        high_priority();
    }

    // Waiting for both children processes to finish
    wait();
    wait();

    printf(1, "[TEST END] Test 3: Priority Inversion (Priority Inheritance)\n");
    exit();
}
