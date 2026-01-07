#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

#define VALID_LOCK_ID 1
#define INVALID_LOCK_ID 7

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

void test_basic_lock_functionality() {
    int pid1, pid2;

    printf(1, "[TEST START] Test 1: Basic Lock Functionality\n");

    pid1 = fork();
    if (pid1 == 0) {
        printf(1, "[Time  %d] Process  %d  attempting to acquire lock %d (Should succeed)\n", get_time(), getpid(), VALID_LOCK_ID);        
        if (lock(VALID_LOCK_ID) == 0) {
            log_result(1, "Process successfully acquired lock");

            sleep(30);

            printf(1, "[Time %d] Process %d releasing lock %d (Should succeed)\n", get_time(), getpid(), VALID_LOCK_ID);            
            if (release(VALID_LOCK_ID) == 0) {
                log_result(1, "Process successfully released lock");
            } 
            else {
                log_result(0, "Failed to release lock");
            }
        } 
        else {
            log_result(0, "Failed to acquire lock");
        }
        exit();
    }

    sleep(5);

    printf(1, "[Time  %d] Parent process attempting to acquire invalid lock  %d\n", get_time(), INVALID_LOCK_ID);
    if (lock(INVALID_LOCK_ID) == -1) {
        log_result(1, "Correctly failed to acquire invalid lock");
    }
    else {
        log_result(0, "Incorrectly acquired invalid lock");
    }

    pid2 = fork();
    if (pid2 == 0) {
        printf(1, "[Time  %d] Process  %d  attempting to acquire lock %d (Should wait if lock is held, then acquire)\n", get_time(), getpid(), VALID_LOCK_ID);
        if (lock(VALID_LOCK_ID) == 0) {
            log_result(1, "Second process successfully acquired lock after waiting");

            printf(1, "[Time  %d] Process  %d  releasing lock %d (Should succeed)\n", get_time(), getpid(), VALID_LOCK_ID);
            if (release(VALID_LOCK_ID) == 0) {
                log_result(1, "Second process successfully released lock");
            } 
            else {
                log_result(0, "Failed to release lock");
            }
        } 
        else {
            log_result(0, "Second process failed to acquire lock");
        }
        exit();
    }

    wait();
    wait();

    printf(1, "[TEST END] Test 1: Basic Lock Functionality\n");
}

int main() {
    test_basic_lock_functionality();
    exit();
}