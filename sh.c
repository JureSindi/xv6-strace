// Shell.

#include "types.h"
#include "user.h"
#include "fcntl.h"
#include "strace.h"

// Parsed command representation
#define EXEC  1
#define REDIR 2
#define PIPE  3
#define LIST  4
#define BACK  5

#define MAXARGS 10

const char *syscall_names[22] = {"fork","exit","wait","pipe","read","kill","exec","fstat","chdir","dup","getpid","sbrk","sleep","uptime","open","write","mknod","unlink","link","mkdir","close","trace"};

int stracing = 0;
char straceRun[] = "strace on\n";
char unstraceRun[] = "strace off\n";

int dumping = 0;
char straceDump[] = "strace dump\n";

struct cmd {
  int type;
};

struct execcmd {
  int type;
  char *argv[MAXARGS];
  char *eargv[MAXARGS];
};

struct redircmd {
  int type;
  struct cmd *cmd;
  char *file;
  char *efile;
  int mode;
  int fd;
};

struct pipecmd {
  int type;
  struct cmd *left;
  struct cmd *right;
};

struct listcmd {
  int type;
  struct cmd *left;
  struct cmd *right;
};

struct backcmd {
  int type;
  struct cmd *cmd;
};

int fork1(void);  // Fork but panics on failure.
void panic(char*);
struct cmd *parsecmd(char*);

// Execute cmd.  Never returns.
void
runcmd(struct cmd *cmd)
{
  int p[2];
  struct backcmd *bcmd;
  struct execcmd *ecmd;
  struct listcmd *lcmd;
  struct pipecmd *pcmd;
  struct redircmd *rcmd;

  if(cmd == 0)
    exit();

  switch(cmd->type){
  default:
    panic("runcmd");

  case EXEC:
    ecmd = (struct execcmd*)cmd;
    if(ecmd->argv[0] == 0)
      exit();
    if (stracing) {
      strace(T_STRACE | T_FORK);
    }
    exec(ecmd->argv[0], ecmd->argv);
    printf(2, "exec %s failed\n", ecmd->argv[0]);
    break;

  case REDIR:
    rcmd = (struct redircmd*)cmd;
    close(rcmd->fd);
    if(open(rcmd->file, rcmd->mode) < 0){
      printf(2, "open %s failed\n", rcmd->file);
      exit();
    }
    runcmd(rcmd->cmd);
    break;

  case LIST:
    lcmd = (struct listcmd*)cmd;
    if(fork1() == 0)
      runcmd(lcmd->left);
    wait();
    runcmd(lcmd->right);
    break;

  case PIPE:
    pcmd = (struct pipecmd*)cmd;
    if(pipe(p) < 0)
      panic("pipe");
    if(fork1() == 0){
      close(1);
      dup(p[1]);
      close(p[0]);
      close(p[1]);
      runcmd(pcmd->left);
    }
    if(fork1() == 0){
      close(0);
      dup(p[0]);
      close(p[0]);
      close(p[1]);
      runcmd(pcmd->right);
    }
    close(p[0]);
    close(p[1]);
    wait();
    wait();
    break;

  case BACK:
    bcmd = (struct backcmd*)cmd;
    if(fork1() == 0)
      runcmd(bcmd->cmd);
    break;
  }
  exit();
}

int
getcmd(char *buf, int nbuf)
{
  printf(2, "$ ");
  memset(buf, 0, nbuf);
  gets(buf, nbuf);
  if(buf[0] == 0) // EOF
    return -1;
  return 0;
}

// This was implemented because I couldn't use strncpy
int streq(char* s1, char* s2) {
    while(1) {
      if(*s1 != *s2) {
        return 0;
      }
      if(*s1 == '\n') {
        return 1;
      }
      s1++;
      s2++;
    }
}

int straceR_commands(char* buf, int* straceRunning, int* straceFlag);
int straceO_commands(char* buf, int* straceRunning, int* straceFlag);
int straceR_mode(char* buf, int* straceRunning, int* straceFlag);

int main(void)
{
  static char buf[100];
  int fd;

  // Ensure that three file descriptors are open.
  while((fd = open("console", O_RDWR)) >= 0){
    if(fd >= 3){
      close(fd);
      break;
    }
  }

  int straceRunning = 0;
  int straceFlag = 0;

  // Read and run input commands.
  while(getcmd(buf, sizeof(buf)) >= 0){
    if(buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' '){
      // Chdir must be called by the parent, not the child.
      buf[strlen(buf)-1] = 0;  // chop \n
      if(chdir(buf+3) < 0)
        printf(2, "cannot cd %s\n", buf+3);
      continue;
    }

    if(straceR_commands(buf, &straceRunning, &straceFlag))
      continue;

    if(straceO_commands(buf, &straceRunning, &straceFlag))
      continue;

    if(straceR_mode(buf, &straceRunning, &straceFlag))
      continue;

    if(straceRunning == 0 && fork1() == 0) 
      runcmd(parsecmd(buf));
    else
      wait();

    if(straceFlag == 1)
      stracing = 0;
    else
      straceFlag = 0;

    if(straceRunning == 1)
      stracing = 0;
    else
      straceRunning = 0;
  }
  
  exit();
}

int parse_syscallN(char* buf, int start_index, char* syscallCmd) {
  int i;
  for(i = 0; i < 20; i++)
    syscallCmd[i] = '\0';

  for(i = start_index; i < strlen(buf)-1; i++)
    syscallCmd[i - start_index] = buf[i];

  int syscallIndex = -1;
  for(i = 0; i < 22; i++) {
    if(strcmp(syscallCmd, syscall_names[i]) == 0) {
      syscallIndex = i;
    }
  }
  
  return syscallIndex;
}

int straceR_commands(char* buf, int* straceRunning, int* straceFlag) {
  if(streq(buf, straceRun)) {
    stracing = 1;
    return 1;
  }
  else if(streq(buf, unstraceRun)) {
    stracing = 0;
    return 1;
  }
  else if((dumping == 1) || streq(buf, straceDump)) {
    dumping = 1;
    if(fork1() == 0) {
        runcmd(parsecmd(buf + 7));
    }
    wait();
    dumping = 0;
    return 1;
  }
  return 0;
}

int straceO_commands(char* buf, int* straceRunning, int* straceFlag) {
  // Strace -e syscall
  if(buf[0] == 's' && buf[1] == 't' && buf[2] == 'r' && buf[3] == 'a' && buf[4] == 'c' && buf[5] == 'e' && buf[6] == ' ' && buf[7] == '-' && buf[8] == 'e' && buf[9] == ' ') {
    char syscallCmd[20];
    int syscallIndex = parse_syscallN(buf, 10, syscallCmd);
    
    stracing = 1;
    *straceFlag = 1;
    eflag(syscallIndex);
    return 1;
  }
  
  // Strace -s -e syscall
  else if(buf[0] == 's' && buf[1] == 't' && buf[2] == 'r' && buf[3] == 'a' && buf[4] == 'c' && buf[5] == 'e' && buf[6] == ' ' && buf[7] == '-' && buf[8] == 's' && buf[9] == ' ' && buf[10] == '-' && buf[11] == 'e' && buf[12] == ' ') {
    char syscallCmd[20];
    int syscallIndex = parse_syscallN(buf, 13, syscallCmd);
    
    stracing = 1;
    *straceFlag = 1;
    sflag(1);
    eflag(syscallIndex);
    return 1;
  }
  
  // Strace -f -e syscall
  else if(buf[0] == 's' && buf[1] == 't' && buf[2] == 'r' && buf[3] == 'a' && buf[4] == 'c' && buf[5] == 'e' && buf[6] == ' ' && buf[7] == '-' && buf[8] == 'f' && buf[9] == ' ' && buf[10] == '-' && buf[11] == 'e' && buf[12] == ' ') {
    char syscallCmd[20];
    int syscallIndex = parse_syscallN(buf, 13, syscallCmd);
    
    stracing = 1;
    *straceFlag = 1;
    fflag(1);
    eflag(syscallIndex);
    return 1;
  }
  
  // Strace -s
  else if(buf[0] == 's' && buf[1] == 't' && buf[2] == 'r' && buf[3] == 'a' && buf[4] == 'c' && buf[5] == 'e' && buf[6] == ' ' && buf[7] == '-' && buf[8] == 's') {
    stracing = 1;
    *straceFlag = 1;
    sflag(1);
    return 1;
  }
  
  // Strace -f
  else if(buf[0] == 's' && buf[1] == 't' && buf[2] == 'r' && buf[3] == 'a' && buf[4] == 'c' && buf[5] == 'e' && buf[6] == ' ' && buf[7] == '-' && buf[8] == 'f') {
    stracing = 1;
    *straceFlag = 1;
    fflag(1);
    return 1;
  }
  
  return 0;
}

int straceR_mode(char* buf, int* straceRunning, int* straceFlag) {
  if(*straceRunning == 1 || (buf[0] == 's' && buf[1] == 't' && buf[2] == 'r' && buf[3] == 'a' && buf[4] == 'c' && buf[5] == 'e' && buf[6] == ' ' && buf[7] == 'r' && buf[8] == 'u' && buf[9] == 'n' && buf[10] == ' ')) {
    stracing = 1;
    *straceRunning = 1;
      
    if(fork1() == 0)
      runcmd(parsecmd(buf + 11));
    else
      wait();
    
    return 1;
  }
  return 0;
}

void
panic(char *s)
{
  printf(2, "%s\n", s);
  exit();
}

int
fork1(void)
{
  int pid;

  pid = fork();
  if(pid == -1)
    panic("fork");
  return pid;
}

//PAGEBREAK!
// Constructors

struct cmd*
execcmd(void)
{
  struct execcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = EXEC;
  return (struct cmd*)cmd;
}

struct cmd*
redircmd(struct cmd *subcmd, char *file, char *efile, int mode, int fd)
{
  struct redircmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = REDIR;
  cmd->cmd = subcmd;
  cmd->file = file;
  cmd->efile = efile;
  cmd->mode = mode;
  cmd->fd = fd;
  return (struct cmd*)cmd;
}

struct cmd*
pipecmd(struct cmd *left, struct cmd *right)
{
  struct pipecmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = PIPE;
  cmd->left = left;
  cmd->right = right;
  return (struct cmd*)cmd;
}

struct cmd*
listcmd(struct cmd *left, struct cmd *right)
{
  struct listcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = LIST;
  cmd->left = left;
  cmd->right = right;
  return (struct cmd*)cmd;
}

struct cmd*
backcmd(struct cmd *subcmd)
{
  struct backcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = BACK;
  cmd->cmd = subcmd;
  return (struct cmd*)cmd;
}
//PAGEBREAK!
// Parsing

char whitespace[] = " \t\r\n\v";
char symbols[] = "<|>&;()";

int
gettoken(char **ps, char *es, char **q, char **eq)
{
  char *s;
  int ret;

  s = *ps;
  while(s < es && strchr(whitespace, *s))
    s++;
  if(q)
    *q = s;
  ret = *s;
  switch(*s){
  case 0:
    break;
  case '|':
  case '(':
  case ')':
  case ';':
  case '&':
  case '<':
    s++;
    break;
  case '>':
    s++;
    if(*s == '>'){
      ret = '+';
      s++;
    }
    break;
  default:
    ret = 'a';
    while(s < es && !strchr(whitespace, *s) && !strchr(symbols, *s))
      s++;
    break;
  }
  if(eq)
    *eq = s;

  while(s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return ret;
}

int
peek(char **ps, char *es, char *toks)
{
  char *s;

  s = *ps;
  while(s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return *s && strchr(toks, *s);
}

struct cmd *parseline(char**, char*);
struct cmd *parsepipe(char**, char*);
struct cmd *parseexec(char**, char*);
struct cmd *nulterminate(struct cmd*);

struct cmd*
parsecmd(char *s)
{
  char *es;
  struct cmd *cmd;

  es = s + strlen(s);
  cmd = parseline(&s, es);
  peek(&s, es, "");
  if(s != es){
    printf(2, "leftovers: %s\n", s);
    panic("syntax");
  }
  nulterminate(cmd);
  return cmd;
}

struct cmd*
parseline(char **ps, char *es)
{
  struct cmd *cmd;

  cmd = parsepipe(ps, es);
  while(peek(ps, es, "&")){
    gettoken(ps, es, 0, 0);
    cmd = backcmd(cmd);
  }
  if(peek(ps, es, ";")){
    gettoken(ps, es, 0, 0);
    cmd = listcmd(cmd, parseline(ps, es));
  }
  return cmd;
}

struct cmd*
parsepipe(char **ps, char *es)
{
  struct cmd *cmd;

  cmd = parseexec(ps, es);
  if(peek(ps, es, "|")){
    gettoken(ps, es, 0, 0);
    cmd = pipecmd(cmd, parsepipe(ps, es));
  }
  return cmd;
}

struct cmd*
parseredirs(struct cmd *cmd, char **ps, char *es)
{
  int tok;
  char *q, *eq;

  while(peek(ps, es, "<>")){
    tok = gettoken(ps, es, 0, 0);
    if(gettoken(ps, es, &q, &eq) != 'a')
      panic("missing file for redirection");
    switch(tok){
    case '<':
      cmd = redircmd(cmd, q, eq, O_RDONLY, 0);
      break;
    case '>':
      cmd = redircmd(cmd, q, eq, O_WRONLY|O_CREATE, 1);
      break;
    case '+':  // >>
      cmd = redircmd(cmd, q, eq, O_WRONLY|O_CREATE, 1);
      break;
    }
  }
  return cmd;
}

struct cmd*
parseblock(char **ps, char *es)
{
  struct cmd *cmd;

  if(!peek(ps, es, "("))
    panic("parseblock");
  gettoken(ps, es, 0, 0);
  cmd = parseline(ps, es);
  if(!peek(ps, es, ")"))
    panic("syntax - missing )");
  gettoken(ps, es, 0, 0);
  cmd = parseredirs(cmd, ps, es);
  return cmd;
}

struct cmd*
parseexec(char **ps, char *es)
{
  char *q, *eq;
  int tok, argc;
  struct execcmd *cmd;
  struct cmd *ret;

  if(peek(ps, es, "("))
    return parseblock(ps, es);

  ret = execcmd();
  cmd = (struct execcmd*)ret;

  argc = 0;
  ret = parseredirs(ret, ps, es);
  while(!peek(ps, es, "|)&;")){
    if((tok=gettoken(ps, es, &q, &eq)) == 0)
      break;
    if(tok != 'a')
      panic("syntax");
    cmd->argv[argc] = q;
    cmd->eargv[argc] = eq;
    argc++;
    if(argc >= MAXARGS)
      panic("too many args");
    ret = parseredirs(ret, ps, es);
  }
  cmd->argv[argc] = 0;
  cmd->eargv[argc] = 0;
  return ret;
}

// NUL-terminate all the counted strings.
struct cmd*
nulterminate(struct cmd *cmd)
{
  int i;
  struct backcmd *bcmd;
  struct execcmd *ecmd;
  struct listcmd *lcmd;
  struct pipecmd *pcmd;
  struct redircmd *rcmd;

  if(cmd == 0)
    return 0;

  switch(cmd->type){
  case EXEC:
    ecmd = (struct execcmd*)cmd;
    for(i=0; ecmd->argv[i]; i++)
      *ecmd->eargv[i] = 0;
    break;

  case REDIR:
    rcmd = (struct redircmd*)cmd;
    nulterminate(rcmd->cmd);
    *rcmd->efile = 0;
    break;

  case PIPE:
    pcmd = (struct pipecmd*)cmd;
    nulterminate(pcmd->left);
    nulterminate(pcmd->right);
    break;

  case LIST:
    lcmd = (struct listcmd*)cmd;
    nulterminate(lcmd->left);
    nulterminate(lcmd->right);
    break;

  case BACK:
    bcmd = (struct backcmd*)cmd;
    nulterminate(bcmd->cmd);
    break;
  }
  return cmd;
}
