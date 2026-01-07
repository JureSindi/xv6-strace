#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

#define MAX_LINES 100
#define MAX_LINE_LENGTH 512

void print_lastlines (char *lines[], int n, int lineCount) {
    int start = lineCount > n ? lineCount - n : 0;

    int i;
    for (i = start; i < lineCount; i++) {
        printf(1, "%s", lines[i]);
    }
}

void read_and_storelines (int fd, char *lines[], int *lineCount, int maxLines, int maxLineLength) {
    char line[MAX_LINE_LENGTH];
    while (1) {
        int i = 0;
        char c;
        while (read(fd, &c, 1) ==  1) {
            if (c == '\n' || i == maxLineLength - 2) {
                line[i++] = c;
                line[i] = '\0';
                break; 
            }
            line[i++] = c;
        }
        if (i == 0)
            break;

        lines[*lineCount] = malloc(strlen(line) + 1);
        strcpy(lines[*lineCount], line);
        (*lineCount)++;

        if (*lineCount == maxLines)
            break;
    }
}

int main (int argc, char *argv[]) {
    int lineCount = 0;
    int n = 10; // The default line count is 10
    char *lines[MAX_LINES];
    int fd;

    if (argc > 1 && argv[1][0] == '-') {
        n = atoi(argv[1] + 1);
        argv++;
        argc--;
    }

    if (argc == 1) {
        read_and_storelines(0, lines, &lineCount, MAX_LINES, MAX_LINE_LENGTH);
    }
    else {
        if ((fd = open(argv[1], O_RDONLY)) < 0 ) {
            printf(2, "tail: can't be opened :( %s\n", argv[1]);
            exit();
        }
        read_and_storelines(fd, lines, &lineCount, MAX_LINES, MAX_LINE_LENGTH);
        close(fd);
    }
    print_lastlines(lines, n, lineCount);

    // Now I will be freeing the allocated memory
    int i;
    for (i = 0; i < lineCount; i++) {
        free(lines[i]);
    }
    exit();
}