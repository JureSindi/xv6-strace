# xv6 Kernel Extensions: strace, Priority Scheduler & Priority Inheritance
 
A set of original kernel-level extensions built directly into the xv6 operating system, covering system call tracing, process priority management, a round-robin priority scheduler with five levels, and a full priority inversion/inheritance implementation.
 
## Why this matters
 
Most OS coursework stops at reading kernel code. This project writes it — adding non-trivial scheduler policy and diagnostic infrastructure to a real (if small) Unix-like kernel. Priority inversion is a class of bug responsible for real-world failures (including the Mars Pathfinder mission); implementing its resolution via priority inheritance requires understanding lock ownership, scheduling queues, and process state at the kernel level. The strace tracer adds runtime visibility into syscall behavior, applied here to diagnose a real memory-leak program.
 
---
 
## Part 1 - `nice` System Call & Process Priority
 
Implemented a `nice` system call that allows a process to adjust its own scheduling priority by modifying its nice value. Higher nice value = lower priority, consistent with UNIX semantics.
 
**What was implemented:**
- New `nice` system call registered in the xv6 syscall table
- Process struct extended to carry a `nice` field
- `nice.c` userspace program to invoke and test the system call
**How to run:**
```bash
make clean && make
make qemu-nox
 
# Inside xv6 shell:
nice 1 4      # set process 1 to nice value 4
```
 
---
 
## Part 2 - Round-Robin Priority Scheduler (5 Levels)
 
Replaced xv6's default round-robin scheduler with a priority-aware variant supporting five priority levels. Processes at higher priority levels are scheduled before lower-priority ones; within the same level, round-robin ordering is preserved.
 
**What was implemented:**
- Priority-aware scheduler loop replacing the default xv6 scheduler
- Five priority levels controlled via the `nice` value
- Compile-time toggle: `CFLAGS = -DSCHEDULER_PRIORITY` enables priority scheduling; removing it restores standard round-robin
- Test programs (`test1`, `test2`, `test3`) fork processes at different priority levels performing computational work, demonstrating scheduler behavior
**How to run:**
```bash
# Enable priority scheduler
# In Makefile: CFLAGS = -DSCHEDULER_PRIORITY
 
make clean && make
make qemu-nox
 
# Inside xv6 shell:
test1    # or test2, test3
```
 
To revert to standard round-robin, remove `-DSCHEDULER_PRIORITY` from `CFLAGS` and recompile.
 
---
 
## Part 3 - Priority Inversion & Inheritance (Extra Credit)
 
Implemented priority inversion detection and resolution via priority inheritance — the same mechanism used to fix the Mars Pathfinder scheduler bug. When a high-priority process is blocked waiting on a lock held by a low-priority process, the low-priority process temporarily inherits the high-priority process's priority until the lock is released.
 
**What was implemented:**
- Lock ownership tracking in the kernel (which process holds which lock)
- Priority inheritance: low-priority lock holder receives elevated priority when a higher-priority process is waiting
- Priority restoration: original priority restored on lock release
- Test programs (`etest1`, `etest2`, `etest3`) simulate priority inversion scenarios and verify that inheritance resolves the inversion correctly
**How to run:**
```bash
make clean && make
make qemu-nox
 
# Inside xv6 shell:
etest1    # or etest2, etest3
# Observe: low-priority process inherits high-priority process's priority
#          when holding a contested lock
```
 
---
 
## Part 4 - `strace` System Call Tracer
 
A `strace`-style system call tracer built into the xv6 kernel, supporting per-command, per-process, and global tracing modes with circular buffer logging and output redirection.
 
**Modes:**
| Command | Behavior |
|---|---|
| `strace on` | Enable global tracing for all processes |
| `strace off` | Disable global tracing |
| `strace run <cmd>` | Trace a single command only |
| `strace dump` | Print circular buffer of recent syscall history |
| `strace run -o <file> <cmd>` | Redirect trace output to a file |
 
**Applied diagnostic case:**
The tracer was applied to a known memory-leak program. It successfully identified repeated failing `sbrk()` calls followed by `SIGKILL` termination, a result that would be invisible without kernel-level syscall visibility.
 
**How to run:**
```bash
make clean && make
make qemu-nox
 
# Inside xv6 shell:
strace on
strace run ls
strace dump
strace off
```
 
---
 
## Repository structure
 
```
xv6-public/
├── kernel/
│   ├── proc.c          # Scheduler, nice, priority inheritance
│   ├── proc.h          # Process struct (nice field, lock owner)
│   ├── syscall.c       # Syscall dispatch table
│   ├── sysproc.c       # nice() and strace() implementations
│   └── trap.c          # Trap/interrupt handling
├── user/
│   ├── nice.c          # nice userspace program
│   ├── test1.c         # Priority scheduler test
│   ├── test2.c
│   ├── test3.c
│   ├── etest1.c        # Priority inheritance test
│   ├── etest2.c
│   └── etest3.c
└── Makefile
```
 
## Skills demonstrated
 
- Kernel-level C development (xv6, RISC-V)
- Scheduler design: round-robin, priority queues, preemption policy
- Synchronization primitives: lock ownership tracking, priority inheritance
- System call implementation (registration, dispatch, userspace interface)
- Runtime debugging via kernel-instrumented syscall tracing
## Context
 
Built for NYU Tandon's graduate Introduction to Operating Systems course (Fall 2024). All three components, the scheduler, inheritance mechanism, and tracer, are original extensions to the xv6 baseline with no reference implementation provided.
 
