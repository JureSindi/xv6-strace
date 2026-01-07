#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "syscall.h"
#include "strace.h"

extern int e_flag;
extern int s_flag;
extern int f_flag;

int e_flag = -1;;
int s_flag = 0;
int f_flag = 0;

// User code makes a system call with INT T_SYSCALL.
// System call number in %eax.
// Arguments on the stack, from the user call to the C
// library system call function. The saved user %esp points
// to a saved program counter, and then the first argument.

// Fetch the int at addr from the current process.
int
fetchint(uint addr, int *ip)
{
  if(addr >= proc->sz || addr+4 > proc->sz)
    return -1;
  *ip = *(int*)(addr);
  return 0;
}

// Fetch the nul-terminated string at addr from the current process.
// Doesn't actually copy the string - just sets *pp to point at it.
// Returns length of string, not including nul.
int
fetchstr(uint addr, char **pp)
{
  char *s, *ep;

  if(addr >= proc->sz)
    return -1;
  *pp = (char*)addr;
  ep = (char*)proc->sz;
  for(s = *pp; s < ep; s++)
    if(*s == 0)
      return s - *pp;
  return -1;
}

// Fetch the nth 32-bit system call argument.
int
argint(int n, int *ip)
{
  return fetchint(proc->tf->esp + 4 + 4*n, ip);
}

// Fetch the nth word-sized system call argument as a pointer
// to a block of memory of size bytes.  Check that the pointer
// lies within the process address space.
int
argptr(int n, char **pp, int size)
{
  int i;

  if(argint(n, &i) < 0)
    return -1;
  if(size < 0 || (uint)i >= proc->sz || (uint)i+size > proc->sz)
    return -1;
  *pp = (char*)i;
  return 0;
}

// Fetch the nth word-sized system call argument as a string pointer.
// Check that the pointer is valid and the string is nul-terminated.
// (There is no shared writable memory, so the string can't change
// between this check and being used by the kernel.)
int
argstr(int n, char **pp)
{
  int addr;
  if(argint(n, &addr) < 0)
    return -1;
  return fetchstr(addr, pp);
}

extern int sys_chdir(void);
extern int sys_close(void);
extern int sys_dup(void);
extern int sys_exec(void);
extern int sys_exit(void);
extern int sys_fork(void);
extern int sys_fstat(void);
extern int sys_getpid(void);
extern int sys_kill(void);
extern int sys_link(void);
extern int sys_mkdir(void);
extern int sys_mknod(void);
extern int sys_open(void);
extern int sys_pipe(void);
extern int sys_read(void);
extern int sys_sbrk(void);
extern int sys_sleep(void);
extern int sys_unlink(void);
extern int sys_wait(void);
extern int sys_write(void);
extern int sys_uptime(void);
extern int sys_nice(void);
extern int sys_lock(void);
extern int sys_release(void);
extern int sys_strace(void);
extern int sys_dump(void);
extern int sys_eflag(void);
extern int sys_sflag(void);
extern int sys_fflag(void);

static int (*syscalls[])(void) = {
[SYS_fork]    sys_fork,
[SYS_exit]    sys_exit,
[SYS_wait]    sys_wait,
[SYS_pipe]    sys_pipe,
[SYS_read]    sys_read,
[SYS_kill]    sys_kill,
[SYS_exec]    sys_exec,
[SYS_fstat]   sys_fstat,
[SYS_chdir]   sys_chdir,
[SYS_dup]     sys_dup,
[SYS_getpid]  sys_getpid,
[SYS_sbrk]    sys_sbrk,
[SYS_sleep]   sys_sleep,
[SYS_uptime]  sys_uptime,
[SYS_open]    sys_open,
[SYS_write]   sys_write,
[SYS_mknod]   sys_mknod,
[SYS_unlink]  sys_unlink,
[SYS_link]    sys_link,
[SYS_mkdir]   sys_mkdir,
[SYS_close]   sys_close,
[SYS_nice]    sys_nice,
[SYS_lock]    sys_lock,
[SYS_release] sys_release,
[SYS_strace]   sys_strace, 
[SYS_dump]    sys_dump,
[SYS_eflag]   sys_eflag, 
[SYS_sflag]   sys_sflag,
[SYS_fflag]   sys_fflag,  
};

// This was very much needed
static char* syscallnames[] = {
    [SYS_fork]  "fork",
    [SYS_exit]  "exit",
    [SYS_wait]  "wait",
    [SYS_pipe]  "pipe",
    [SYS_read]  "read",
    [SYS_kill]  "kill",
    [SYS_exec]  "exec",
    [SYS_fstat] "fstat",
    [SYS_chdir] "chdir",
    [SYS_dup]   "dup",
    [SYS_getpid] "getpid",
    [SYS_sbrk]  "sbrk",
    [SYS_sleep] "sleep",
    [SYS_uptime] "uptime",
    [SYS_open]  "open",
    [SYS_write] "write",
    [SYS_mknod] "mknod",
    [SYS_unlink] "unlink",
    [SYS_link]   "link",
    [SYS_mkdir]  "mkdir",
    [SYS_close]  "close",
    [SYS_strace] "trace",
    [SYS_dump]   "dump",
};

char dumpBuf[N][100];

int startIndex=0;
int* startPointer = &startIndex;

int wrap =0;
int* wrapPointer = &wrap;

int endIndex=0;
int* endPointer = &endIndex;

void 
adding(char* straceBuf) {
    int start = *startPointer;
    int end = *endPointer;
    int i = 0;
    int wrapped = *wrapPointer;

    if(start == end && wrap) {
        memset(dumpBuf[end],0,strlen(dumpBuf[end]));
        end = (end + 1) % N;
    }
    while(straceBuf[i] != '\0') {
        dumpBuf[start][i] = straceBuf[i];
        i += 1;
    }
    start= (start + 1) % N;
    if(start == 0) {
        wrapped = 1;
    }
    *startPointer = start;
    *endPointer = end;
    *wrapPointer = wrapped;
}

int 
compare(char s1[], char s2[]) {
    int index = 0;
    while(s1[index] == s2[index]) {
        if(s1[index]=='\0'||s2[index]=='\0')
            break;
        index++;
    }
    if(s1[index]=='\0' && s2[index]=='\0'){
        return 0;
    }
    else {
        return -1;
    }
}

void 
pid_to_string(int pid, char* pidS) {
    int i = 0;

    while(pid != 0) {
        int k = pid % 10;
        pidS[i] = k + '0';
        i += 1;
        pid /= 10;
    }
}

void 
string(char* straceBuf, int* i, const char* s) {
    int j = 0;

    while(s[j] != '\0') {
        straceBuf[*i] = s[j];
        j += 1;
        *i += 1;
    }
}

void 
number_to_string(char* straceBuf, int* i, int num) {
    if(num == 0) {
        straceBuf[*i] = num + '0';
        *i += 1;
    } 
    else {
        char pidS[5];
        int j = 0;
        while(num != 0) {
            int k = num % 10;
            pidS[j] = k + '0';
            j += 1;
            num /= 10;
        }
        j -= 1;
        while(j >= 0) {
            straceBuf[*i] = pidS[j];
            *i += 1;
            j -= 1;
        }
    }
}

void 
create_strace(char* straceBuf, int pid, const char* processN, int syscallNum) {
    int i = 0;
    char s1[50] = "TRACE: pid = ";
    char s2[50] = " | command name = ";
    char s3[50] = " | syscall = ";
    char pidS[5] = {0};

    string(straceBuf, &i, s1);
    pid_to_string(pid, pidS);

    int index = 0;

    while(pidS[index] != '\0') index++;
    index--;
    
    while(index >= 0) {
        straceBuf[i] = pidS[index];
        i++;
        index--;
    }

    straceBuf[i] = ' ';
    i++;
    string(straceBuf, &i, s2);

    straceBuf[i] = ' ';
    i++;
    string(straceBuf, &i, processN);

    straceBuf[i] = ' ';
    i++;
    string(straceBuf, &i, s3);
    
    straceBuf[i] = ' ';
    i++;
    string(straceBuf, &i, syscallnames[syscallNum]);

    straceBuf[i] = '\0';
}

void 
strace_return(char* straceBuf, int pid, const char* processN, int syscallNum, int returnV) {
    int i = 0;
    char s1[50] = "TRACE: pid = ";
    char s2[50] = " | command name = ";
    char s3[50] = " | syscall = ";
    char s5[50] = " | return value = ";
    char pidS[5] = {0};

    string(straceBuf, &i, s1);
    pid_to_string(pid, pidS);
    
    int index = 0;

    while(pidS[index] != '\0') index++;
    index--;

    while(index >= 0) {
        straceBuf[i] = pidS[index];
        i++;
        index--;
    }

    straceBuf[i] = ' ';
    i++;
    string(straceBuf, &i, s2);

    straceBuf[i] = ' ';
    i++;
    string(straceBuf, &i, processN);

    straceBuf[i] = ' ';
    i++;
    string(straceBuf, &i, s3);

    straceBuf[i] = ' ';
    i++;
    string(straceBuf, &i, syscallnames[syscallNum]);

    straceBuf[i] = ' ';
    i++;
    string(straceBuf, &i, s5);
    number_to_string(straceBuf, &i, returnV);

    straceBuf[i] = '\0';
}

int 
sys_strace(void) {
    int i = 0;
    argint(0, &i);
    proc->straced = (i & T_STRACE) ? i:0;
    return 0;
}

int 
sys_dump(void) {
  int i;
  for(i = 0; i < N && dumpBuf[i]!= 0; i += 1) {
      cprintf("%s\n", dumpBuf[i]);
  }
  return 0;
}

int 
sys_eflag(void) {
    argint(0, &e_flag);
    return 0;
}

int 
sys_sflag(void) {
    argint(0, &s_flag);
    return 0;
}

int 
sys_fflag(void) {
    argint(0, &f_flag);
    return 0;
}

void 
syscall(void) {
    int i, j;
    int syscallNum1, syscallNum2;
    int straced = (proc->straced & T_STRACE);
    int processS = 0;

    for(j = 0; proc->name[j] != 0; j += 1) {
        processS += 1;
    }

    char processN[processS];
    for(i = 0; proc->name[i] != 0; i += 1) {
        processN[i] = proc->name[i];
    }

    processN[i] = proc->name[i];
    syscallNum1 = proc->tf->eax;
    syscallNum2 = proc->tf->eax;

    if(syscallNum1 == SYS_exit && straced) {
        char straceBuf[100];

        create_strace(straceBuf, proc->pid, processN, syscallNum2);
        adding(straceBuf);

        if(e_flag == -1 && s_flag == 0 && f_flag == 0)
            cprintf("TRACE: pid = %d | command name = %s | syscall = %s\n", proc->pid, processN, syscallnames[syscallNum2]);
        else {
            if(e_flag != -1) {
                if(compare(syscallnames[e_flag+1], syscallnames[syscallNum2]) == 0) {
                    if(f_flag == 0)
                        cprintf("TRACE: pid = %d | command name = %s | syscall = %s\n", proc->pid, processN, syscallnames[syscallNum2]);
                }
            }
            else if(f_flag == 0)
                cprintf("TRACE: pid = %d | command name = %s | syscall = %s\n", proc->pid, processN, syscallnames[syscallNum2]);
            
            e_flag = -1;
            s_flag = 0;
            f_flag = 0;
        }
    }
    if(syscallNum1 > 0 && syscallNum1 < NELEM(syscalls) && syscalls[syscallNum1]) {
        proc->tf->eax = syscalls[syscallNum1]();

        if(straced) {
            if(syscallNum1 == SYS_exec && proc->tf->eax == 0) {
                char straceBuf[100];

                create_strace(straceBuf, proc->pid, processN, syscallNum2);
                adding(straceBuf);

                if(e_flag == -1 && s_flag == 0 && f_flag == 0) {
                    cprintf("TRACE: pid = %d | command name = sh | syscall = trace | return value = 0\n", proc->pid);
                    cprintf("TRACE: pid = %d | command name = %s | syscall = %s\n", proc->pid, processN, syscallnames[syscallNum2]);
                }
                else {
                    if(e_flag != -1) {
                        if(compare(syscallnames[e_flag+1], syscallnames[syscallNum2]) == 0) {
                            if(f_flag == 0)
                                cprintf("TRACE: pid = %d | command name = %s | syscall = %s\n", proc->pid, processN, syscallnames[syscallNum2]);
                        }
                    }
                    else if(s_flag == 1) {
                        cprintf("TRACE: pid = %d | command name = sh | syscall = trace | return value = 0\n", proc->pid);
                        cprintf("TRACE: pid = %d | command name = %s | syscall = %s\n", proc->pid, processN, syscallnames[syscallNum2]);
                    }
                    else if(f_flag == 1) { }
                }
            }
            else {
                char straceBuf[100];

                strace_return(straceBuf, proc->pid, processN, syscallNum2, proc->tf->eax);
                adding(straceBuf);

                if(e_flag == -1 && s_flag == 0 && f_flag == 0)
                    cprintf("TRACE: pid = %d | command name = %s | syscall = %s | return value = %d\n", proc->pid, processN, syscallnames[syscallNum2], proc->tf->eax);
                else {
                    int value = proc->tf->eax;

                    if(e_flag != -1) {
                        if(compare(syscallnames[e_flag+1], syscallnames[syscallNum2]) == 0) {
                            if(s_flag == 1 && value >= 0) 
                                cprintf("TRACE: pid = %d | command name = %s | syscall = %s | return value = %d\n", proc->pid, processN, syscallnames[syscallNum2], proc->tf->eax);
                            else if(s_flag == 1 && value <= -1) { }
                            else if(f_flag == 1 && value <= -1)
                                cprintf("TRACE: pid = %d | command name = %s | syscall = %s | return value = %d\n", proc->pid, processN, syscallnames[syscallNum2], proc->tf->eax);
                            else if(f_flag == 1 && value >= 0) { }  
                            else
                                cprintf("TRACE: pid = %d | command name = %s | syscall = %s | return value = %d\n", proc->pid, processN, syscallnames[syscallNum2], proc->tf->eax);
                        }
                    }
                    else if(s_flag == 1 && value >= 0)
                        cprintf("TRACE: pid = %d | command name = %s | syscall = %s | return value = %d\n", proc->pid, processN, syscallnames[syscallNum2], proc->tf->eax);
                    else if(f_flag == 1 && value <= -1)
                        cprintf("TRACE: pid = %d | command name = %s | syscall = %s | return value = %d\n", proc->pid, processN, syscallnames[syscallNum2], proc->tf->eax);
                }
            }
        }
    } 
    else {
        cprintf("Unknown syscall: pid = %d, process = %s, syscall number = %d\n", proc->pid, proc->name, syscallNum1);
        proc->tf->eax = -1;
    }
}
