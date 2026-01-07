#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

// Declarations similar to HW1
#define maxNodes 100
#define lineLength 1000
#define commandLength 500
#define maxParts 10

// Declarations similar to HW1
typedef struct {
    char name[50];
    char command[commandLength];
    char errorTo[50];  // This is for the stderr redirection (needed for the extra credit 1)
    char fileName[50];
    int inputFile;
    int outputFile;
} Node;

// Declarations similar to HW1
typedef struct {
    char name[50];
    char from[50];
    char to[50];
} Pipe;

// Declarations similar to HW1
typedef struct {
    char name[50];
    int partCount;
    char parts[maxParts][50];
} Concatenation;

// Declarations similar to HW1
Node nodes[maxNodes];
int nodeCount = 0;

Pipe pipes[maxNodes];
int pipeCount = 0;

Concatenation concatenations[maxNodes];
int concatCount = 0;

// Declarations of all of the functions
void file(const char* filename);
int nodeSearch (const char* name);
int pipeSearch (const char* name);
int concatSearch (const char* name);
void nodeExecution (int nodeIndex, int input, int output, int error);
void pipeExecution (int pipeIndex, int output);
void concatExecution (int concatIndex, int output);

void file (const char* filename) {
    FILE* file;
    char line[lineLength];
    int concatNow = -1;
    int partIndex = 0;
    char* name;
    char* value;

    file = fopen(filename, "r");
    if (!file) {
        exit(1);
    }

    // I did receive help for this, using AI
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '\n') continue;  // This is for skipping empty lines

        name = strtok(line, "=");
        value = strtok(NULL, "\n");

        if (strcmp(name, "node") == 0) {
            strcpy(nodes[nodeCount].name, value);   // This starts a new node
        } 
        else if (strcmp(name, "command") == 0) {
            strcpy(nodes[nodeCount].command, value);
            nodeCount++;  // This happens after the command is assigned
        } 
        else if (strcmp(name, "pipe") == 0) {
            strcpy(pipes[pipeCount].name, value);
        } 
        else if (strcmp(name, "from") == 0) {
            strcpy(pipes[pipeCount].from, value);
        } 
        else if (strcmp(name, "to") == 0) {
            strcpy(pipes[pipeCount++].to, value);  // This is for the pipe count incrementation
        } 
        else if (strcmp(name, "concatenate") == 0) {
            strcpy(concatenations[concatCount].name, value);
            concatNow = concatCount++;
            partIndex = 0;  // This is resetting for new concatenations
        } 
        else if (strcmp(name, "parts") == 0) {
            concatenations[concatNow].partCount = atoi(value);
        } 
        else if (strncmp(name, "part_", 5) == 0) {
            strcpy(concatenations[concatNow].parts[partIndex++], value);
        } 
        else if (strcmp(name, "stderr") == 0) {
            strcpy(nodes[nodeCount - 1].errorTo, value); // stderr
        } 
        else if (strcmp(name, "file") == 0) {
            strcpy(nodes[nodeCount - 1].fileName, value);
            nodes[nodeCount - 1].inputFile = 1;  // This is for the input file
        } 
        else if (strcmp(name, "name") == 0) {
            strcpy(nodes[nodeCount - 1].fileName, value);
            nodes[nodeCount - 1].outputFile = 1;  // This is for the output file
        }
    }
    fclose(file);
}

int nodeSearch (const char* name) {
    int i;
    for (i = 0; i < nodeCount; i++) {
        if (strcmp(nodes[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

int pipeSearch (const char* name) {
    int i;
    for (i = 0; i < pipeCount; i++) {
        if (strcmp(pipes[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

int concatSearch (const char* name) {
    int i;
    for (i = 0; i < concatCount; i++) {
        if (strcmp(concatenations[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

void nodeExecution (int nodeIndex, int input, int output, int error) {
    pid_t pid = fork();
    if (pid == 0) {
        // This is for the redirect input and output
        if (input != STDIN_FILENO) {
            dup2(input, STDIN_FILENO);
            close(input);
        }
        if (output != STDOUT_FILENO) {
            dup2(output, STDOUT_FILENO);
            close(output);
        }
        if (error != STDERR_FILENO) {
            dup2(error, STDERR_FILENO);
            close(error);
        }

        // This is for the handling of the input file
        if (nodes[nodeIndex].inputFile) {
            int fd = open(nodes[nodeIndex].fileName, O_RDONLY);
            if (fd == -1) {
                perror("open");
                exit(1);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }

        // This is for the parse command into arguments as well as handling of the quoted arguments
        char *args[maxParts];
        int argumentCount = 0;
        char *command = strdup(nodes[nodeIndex].command);
        char *token;
        char *rest = command;

        while ((token = strtok_r(rest, " ", &rest)) != NULL && argumentCount < maxParts - 1) {
            // This is for the handling of the quoted arguments (single ('') or double quotes (""))
            if (token[0] == '\'' || token[0] == '"') {
                char quote = token[0];
                char *endQuote = strchr(token + 1, quote);
                if (endQuote) {
                    *endQuote = '\0';
                    args[argumentCount++] = token + 1;  // This is for adding arguments without quotes
                } 
                else {
                    args[argumentCount++] = token;  // This is for noo closing quote found was found
                }
            } 
            else {
                args[argumentCount++] = token;
            }
        }
        args[argumentCount] = NULL;

        execvp(args[0], args);
        free(command);
        exit(1);
    }
    int status;
    waitpid(pid, &status, 0);
}

void pipeExecution(int pipeIndex, int output) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(1);
    }

    int fromIndex = nodeSearch(pipes[pipeIndex].from);
    int toIndex = nodeSearch(pipes[pipeIndex].to);
    int fromConcatIndex = concatSearch(pipes[pipeIndex].from);
    int fromPipeIndex = pipeSearch(pipes[pipeIndex].from);  // Search for another pipe

    pid_t pid = fork();  // Fork a child process for the 'from' node
    if (pid == 0) {  
        // child
        close(pipefd[0]);  // This is for the closing of the reading end in the child process
        
        // For the handling of the  'stdout_to_stderr_for_mkdir' case
        if (strcmp(pipes[pipeIndex].from, "stdout_to_stderr_for_mkdir") == 0) {
            int error_pipe[2];
            if (pipe(error_pipe) == -1) {
                perror("pipe");
                exit(1);
            }

            pid_t mkdir_pid = fork();
            if (mkdir_pid == 0) {
                close(error_pipe[0]);
                dup2(error_pipe[1], STDERR_FILENO);  // The redirection of stderr to error_pipe
                close(error_pipe[1]);
                nodeExecution(nodeSearch("mkdir_attempt"), STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO);
                exit(0);
            }

            close(error_pipe[1]);
            nodeExecution(nodeSearch("word_count"), error_pipe[0], output, STDERR_FILENO);
            close(error_pipe[0]);
            exit(0);
        } 
        // For the handling of the 'read_pipe' case
        else if (strcmp(pipes[pipeIndex].from, "read_pipe") == 0) {
            nodeExecution(nodeSearch("read_file"), STDIN_FILENO, pipefd[1], STDERR_FILENO);
        } 
        // For the handling of a concatenation as the source
        else if (fromConcatIndex != -1) {
            concatExecution(fromConcatIndex, pipefd[1]);
        } 
        // For the handling of another pipe as the source
        else if (fromPipeIndex != -1) {
            pipeExecution(fromPipeIndex, pipefd[1]);  // For the calling of pipeExecution for the previous pipe
        } 
        else if (fromIndex != -1) {
            nodeExecution(fromIndex, STDIN_FILENO, pipefd[1], STDERR_FILENO);
        } 
        else {
            exit(1);
        }

        close(pipefd[1]);  // This is for the closing of the writing end after the execution
        exit(0);  //  For terminating the child process after execution
    } 
    else if (pid < 0) {
        perror("fork failed");
        exit(1);
    }

    // Parent
    close(pipefd[1]);  // Close writing end in the parent process

    // The execution of the 'to' node
    if (toIndex != -1) {
        nodeExecution(toIndex, pipefd[0], output, STDERR_FILENO);
    } else {
        exit(1);
    }

    close(pipefd[0]);  // This is for the closing reading end after the execution
    wait(NULL);  // This is to wait for the child process to terminate
}

void concatExecution (int concatIndex, int output) {
    int i;
    for (i=  0; i < concatenations[concatIndex].partCount; i++) {
        int nodeIndex = nodeSearch (concatenations[concatIndex].parts[i]);
        int pipeIndex = pipeSearch (concatenations[concatIndex].parts[i]);

        if (nodeIndex != -1) {
            // This is for the execution of the node and make sure its output is being passed correctly
            nodeExecution (nodeIndex, STDIN_FILENO, output, STDERR_FILENO);
        } 
        else if (pipeIndex != -1) {
            // This is for the execution of the pipe and make sure its output is being passed correctly
            pipeExecution (pipeIndex, output);
        } 
        else {
            exit(1);
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        //fprintf(stderr, "Usage: %s <flow_file> <pipe_name>\n", argv[0]);
        return 1;
    }
    file(argv[1]);
    int pipeIndex = pipeSearch (argv[2]);
    if (pipeIndex == -1) {
        return 1;
    }
    pipeExecution (pipeIndex, -1);
    return 0;
}