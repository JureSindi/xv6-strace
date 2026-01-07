#include "types.h"
#include "user.h"
#include "stat.h"
#include "memmove.h"
#include "fcntl.h"

// Make sure that the updated xv6 libraries are consistant with the code, since I messed up and used C lib at first!!!!

// Task 2
#define maxDefinitions 100 // This is for the maximum number of variable definitions
#define maxVariable 256 // This is the maximum length acceptable for the varibales name (identifier)
#define maxLength 1024  // This is for setting the maximum length of a line
#define NULL 0 // THIS!!

typedef struct { // This the struct thats holding the variables definition
    char variable[maxVariable];
    char value[maxLength];
} Definition;
Definition definitions[maxDefinitions];
int numDef = 0; // This is to keep track of the number of varibale definitions that are being added to the defintions array

int validity (const char *str) { // here we are checking if the string is an identifier and a valid C identifier at that 
    if (!((*str >= 'a' && *str <= 'z') || (*str >= 'A' && *str <= 'Z') || *str == '_')) { // here we are checking if the character is an alphabetic letter (lowercase, capitilized, _)
        return 0;
    }
    str++;
    while (*str) {
        if (!((*str >= 'a' && *str <= 'z') || (*str >= 'A' && *str <= 'Z') || (*str >= '0' && *str <= '9') || *str == '_')) { // here we are checking if  the character is a letter (lowercase, capitilized, _) or number
            return 0;
        }
        str++;
    }
    return 1; // return true if the string passes all the tests, and false if not
}

void add (const char *definition) { // This is for adding of the defined variables
    char *equal = strchr(definition, '='); // equal is referring to the equal sign
    if (equal == NULL) {  // 
        exit();
    }
    // This is where we split teh variable and value
    char variable[maxVariable];
    strncpy(variable, definition, equal - definition);
    variable[equal - definition] = '\0';

    const char *value = equal + 1;
    addDefine(variable, value);
}

// This is for Task 4 implementations
void addDefine (const char*variable, const char*value) {
    if (numDef >= maxDefinitions) {
        exit();
    }
    if(!validity(variable)) {
        exit();
    }
    strncpy(definitions[numDef].variable, variable, maxVariable); // This is for storing the variable
    strncpy(definitions[numDef].value, value, maxLength); // This is for storing the value
    numDef++;
}

void replacing (char *line) { // This is for replacing the occurances of the defined variables
    char result[maxLength] = "";
    char *position = line;
    char *write = result;

    while (*position) {
        int replaced = 0;
        for (int i = 0; i < numDef; i++) {
            int length = strlen(definitions[i].variable);
            if ((!isalnum(position[length]) && position[length] != '_') && strncmp(position, definitions[i].variable, length) == 0) {
                memmove(write, definitions[i].value, strlen(definitions[i].value));
                write += strlen(definitions[i].value);
                position += length;
                replaced = 1;
                break;
            }
        }
        if (!replaced) {
            *write++ = *position++;
        }
    }
    *write = '\0';
    memmove(line, result, strlen(result) + 1);
}

int parsing (int argc, char *argv[]) { // This is for the process of parsing the definitions
    // ./preprocess <input_file> -D<var1>=<val1> -D<var2>=<val2> … -D<varN>=<valN>
    for (int i = 2; i < argc; i++) {
        if (strncmp(argv[i], "-D", 2) == 0) {
            add(argv[i] + 2);
        } 
        else {
            exit();
        }
    }
}

// This is for Task 4 implementations
void processingDefine (char *line) {
    char variable[maxVariable], value[maxLength];
    if (sscanf(line, "define %s %s", var, val) == 2) {
        addDefine(variable, value);
    }
}

// This is for accepting a text file (or any other file)
// This is for Task 7 implementations as well
void file (const char *filename) {
    int fl = open(filename, O_RDONLY);
    if (fl < 0) {
        return;
    }
    char line[maxLength];
    int i;
    while ((i = read(fl, line, sizeof(line) - 1)) > 0) {
        line[n] = '\n';

        if (strncmp(line, "#define", 7) == 0) {
            processingDefine(line);
        }
        else if (strncmp(line, "include", 8) == 0) {
            char includedFile[maxVariable];
            if (sscanf(line, "#include \"%[^\"]\"", includedFile) == 1 || sscanf(line, "#include <%[^>]>", includedFile) == 1) {
                file(includedFile);
            }
        }
        else {
            replacing(line);
        }
    }
    if (i < 0) {
        exit();
    }
    close(fl);
}

int main (int argc, char *argv[]) {
    // ./preprocess <input_file> -D<var1>=<val1> -D<var2>=<val2> … -D<varN>=<valN>
    if (argc < 2) { // This is to ensure that there are at least 2 arguments so for example ./preprocess data.txt
        exit();
    }
    parsing(argc, argv); // For parsing the command line args
    file(argv[1]); // For processing the file
    return 0;
}