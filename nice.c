#include "types.h"
#include "stat.h"
#include "user.h"

// This is for spawning the processes
void spawn(int target_pid) {
  int pid;

  while ((pid = fork()) > 0) {
    if (pid >= target_pid) {
      break;
    }
    sleep(10);
  }

  if (pid == 0) {
    // Child process
    while(1) {}
  }
}

int
main(int argc, char *argv[])
{
  int pid = -1;
  int value = 0;
  int old_nice;

  if(argc < 2){
    printf(2, "Usage: nice [pid] value\n");
    exit();
  }

  if (argc == 2) {
    // If only one argument is provided, then assume its the value
    value = atoi(argv[1]);
    pid = getpid();
  } 
  else if (argc == 3) {
    pid = atoi(argv[1]);
    value = atoi(argv[2]);
  }

  if(value < 1 || value > 5) {
    printf(2, "Error: nice value must be between 1 and 5\n");
    exit();
  }

  old_nice = nice(pid, value);
  if (old_nice == -1) {
    printf(2, "Process with PID  %d  isn't found.\n", pid, pid);
    spawn(pid);

    old_nice = nice(pid, value);
    if (old_nice == -1) {
      printf(2, "Error: Unable to change nice value for PID %d\n", pid);
      exit();
    }
  }

  printf(1, "%d  %d\n", pid, old_nice);
  exit();
}
