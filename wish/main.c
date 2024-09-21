#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
// #include <stdint.h>

// https://github.com/remzi-arpacidusseau/ostep-projects/tree/master/processes-shell

#define MAX_CLI_ARGS 1024
#define MAX_PATH_SIZE 512

#define NOT_A_BUILTIN 2

#if __STDC_VERSION__ < 201112L || __STDC_NO_ATOMICS__ == 1
#error "C11 atomics not supported"
#endif

// globals, atomic just in case
_Atomic int atomic_exitting = 0;
// TODO: cannot be atomic, use locks
char *atomic_path[MAX_PATH_SIZE] = {"/bin", "/usr/bin"};

void printError() {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

void tryRunningCommand(char *cmdArgs[], size_t cmdArgsSize) { 

    // TODO: run the process
    // see: https://pages.cs.wisc.edu/~remzi/OSTEP/cpu-api.pdf
}

int tryRunningBuiltIns(char *cmdArgs[], size_t cmdArgsSize) {
    if (strcmp(cmdArgs[0], "exit") == 0) {
        atomic_exitting = 1;
        return 1;
    }
    
    if (strcmp(cmdArgs[0], "cd") == 0) {
        if (cmdArgsSize != 2) {
            // too many arguments for cd
            printError();
            return 1;
        }

        if (chdir(cmdArgs[1]) != 0) {
            printError();
        }

        return 1;
    } 
    
    if (strcmp(cmdArgs[0], "path") == 0) {
        if (cmdArgsSize > MAX_PATH_SIZE) {
            // too many arguments for path
            printError();
            return 1;
        }

        for (size_t i = 1; i < cmdArgsSize; i++) {
            atomic_path[i - 1] = cmdArgs[i];
        }

        return 1;
    }

    return NOT_A_BUILTIN; // not a built-in
}

void interpretLine(char *line) {
    size_t cmdArgsSize = 0;
    char *cmdArgs[MAX_CLI_ARGS];
    char *currentToken;

    for (;(currentToken = strsep(&line, " ")) != NULL; cmdArgsSize++) {
        if (cmdArgsSize >= MAX_CLI_ARGS) {
            printError(); // too many arguments
            return;
        }

        cmdArgs[cmdArgsSize] = currentToken;
    }

    if (cmdArgsSize == 0) {
        return 0;
    }

    if (tryRunningBuiltIns(cmdArgs, cmdArgsSize) == NOT_A_BUILTIN) {
        tryRunningCommand(cmdArgs, cmdArgsSize);
    }
}

void removeNewline(char *str) {
    // find the position of the \n
    size_t len = strcspn(str, "\n");
    if (str[len] == '\n') {
        str[len] = '\0';
    }
}

void runInteractiveShell() {
    // includes the newline character
    char *line = NULL;
    size_t lineLength = 0;

    while (!atomic_exitting) {
        printf("wish> ");
        if (getline(&line, &lineLength, stdin) == -1) {
            break;
        }

        removeNewline(line);
        interpretLine(line);
    }

    free(line);
}

int main(int argc, char *argv[]) {
    // char *path = "/bin:/usr/bin";

    if (argc > 2) {
        exit(1);
    } else if (argc == 2) {
        // char *batchFile  = argv[1];
        // read from bath file
        printf("wish: not implemented\n");
        exit(1);
    } else {
        // interactive mode
        // printf("wish: not implemented\n");
        // exit(1); 
        runInteractiveShell();
    }

    return 0;
}

// TODO: running single command from path: `ls`
