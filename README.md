Task 1:
In this part of the homework, I implemented a nice system call that would adjust the priority of the processes based on the new 
value. The nice.c program interacts with the newly implemented nice system call in xv6 to adjust the priority of processes. The 
purpose of the nice system call is to allow a process to adjust its own priority by modifying its nice value. 
The higher the nice value, the lower the priority. 
To compile and run the program, the following steps take place:
    Open a terminal and navigate to the xv6-public directory.
    Then compile the nice.c program using the provided Makefile by running the following: 
    "make clean"
    "make"
Then run the nice program with the following:
    "make qemu-nox"
Then enter the file name by typing in for instance:
    "nice 1 4" 
Then observe the output which demonstrates the behavior of nice. 

Task 2:
In this part of the homework, I implemented a round robin with priority (the round robin is the standard) with five priority 
levels. The nice.c program is used to test the functionality of the round robin with priority (and standard round robin). For this 
task, processes are forked with different priority levels (controlled by the nice value), and they would perform some 
computational work.
Makefile includes the necessary compile-time options. 
CFLAGS = -DSCHEDULER_PRIORITY     to enable the round robin with priority and to disable it (and enable the standard round robin) 
remove -DSCHEDULER_PRIORITY from CFLAGS.
To compile and run, the following steps take place:
    Open a terminal and navigate to the xv6-public directory.
    Then compile the test case program using the provided Makefile by running the following: 
    "make clean"
    "make"
Then run the test case programs with the following:
    "make qemu-nox"
Then enter the files name by typing in for instance:
    "test1" or "test2" or "test3" 
Then observe the output which demonstrates the behavior of the test cases of scheduling in xv6. \

Extra credit:
In this part of the homework, I implemented the priority inversion and inheritance. The tests (which were specified in the 
homework instructions) demonstrate the priority inversion problem and its resolution using priority inheritance.
To compile and run, the following steps take place:
    Open a terminal and navigate to the xv6-public directory.
    Then compile the test case program using the provided Makefile by running the following: 
    "make clean"
    "make"
Then run the test case programs with the following:
    "make qemu-nox"
Then enter the files name by typing in for instance:
    "etest1" or "etest2" or "etest3" 
Then observe the output which demonstrates the behavior of the test cases. These test cases will simulate priority inversion and 
show how the low-priority process inherits the priority of the high-priotity process when needed. 