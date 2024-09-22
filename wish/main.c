#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
// #include <stdint.h>
#include <stdbool.h>

// https://github.com/remzi-arpacidusseau/ostep-projects/tree/master/processes-shell

#define MAX_CLI_ARGS 1024
#define MAX_PATH_SIZE 512

#define NOT_A_BUILTIN 2

#if __STDC_VERSION__ < 201112L || __STDC_NO_ATOMICS__ == 1
#error "C11 atomics not supported"
#endif

// globals, atomic just in case
_Atomic bool atomic_exitting = false;

// TODO: `path` cannot be atomic, use locks to update both `path` and `pathSize`
char *path[MAX_PATH_SIZE] = { 
    "/bin", 
    // "/usr/bin" 
    };
size_t pathSize = 1;


void printError() {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

char *getCommandPath(char *cmd) {
    size_t cmdSize = strlen(cmd);
    char *cmdPath = NULL;
    
    for (size_t i = 0; i < pathSize; i++) {
        // copy the old path element
        char oldPathElem[strlen(path[i]) + 1];  // +1 for null terminator
        strcpy(oldPathElem, path[i]);

        // create the a temp path element with cmd
        size_t cmdPathSize = strlen(oldPathElem) + cmdSize + 2;   // +1 for null terminator, +1 for '/'
        cmdPath = (char *)malloc(cmdPathSize);
        strcpy(cmdPath, oldPathElem);
        strcat(cmdPath, "/");
        strcat(cmdPath, cmd);

        // check if the command is available
        if (access(cmdPath, X_OK) != 0) {
            free(cmdPath);
            cmdPath = NULL;
            continue;
        }

        return cmdPath;
    }

    return cmdPath;
}

void tryRunningCommand(char *cmdArgs[], size_t cmdArgsSize) { 
    // TODO: run the process
    // see: https://pages.cs.wisc.edu/~remzi/OSTEP/cpu-api.pdf

    // step 1: check if command is in path with `access` - done
    // step 2: fork and exec the command - done
    // step 2.5: check built-in commands - done
    // step 3: redirection (TODO)
    // step 4: parallel commands (TODO)
    // step 5: batch file (TODO)

    char *cmdPath = getCommandPath(cmdArgs[0]);
    if (cmdPath == NULL) {
        printError();
        return;
    }

    int rc = fork();
    if (rc < 0) {
        // fork failed, no child process is created
        printError();
    } else if (rc == 0) {
        // child: execute the command

        // close(STDOUT_FILENO);
        // open("./p4.output", O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);

        char *execArgs[cmdArgsSize + 1];
        // command name without path, specifying path is not supported
        execArgs[0] = cmdArgs[0];
        for (size_t i = 1; i < cmdArgsSize; i++) {
            execArgs[i] = cmdArgs[i];
        }
        execArgs[cmdArgsSize] = NULL;

        if (execv(cmdPath, execArgs)) {
            printError();
        }

    } else {
        // parent: wait for the child to finish
        if (waitpid(rc, NULL, 0) == -1) {
            printError();
        }
    }

    free(cmdPath);
}

int tryRunningBuiltIns(char *cmdArgs[], size_t cmdArgsSize) {
    if (strcmp(cmdArgs[0], "exit") == 0) {
        atomic_exitting = true;
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

        // clear path
        pathSize = 0;
        memset(path, 0, sizeof(path));

        for (size_t i = 1; i < cmdArgsSize; i++) {
            path[i - 1] = cmdArgs[i];
        }
        pathSize = cmdArgsSize - 1;

        return 1;
    }

    return NOT_A_BUILTIN; // not a built-in
}

void interpretLine(char *line) {
    size_t cmdArgsSize = 0;
    char *cmdArgs[MAX_CLI_ARGS];

    for (char *currentToken;(currentToken = strsep(&line, " ")) != NULL; cmdArgsSize++) {
        if (cmdArgsSize >= MAX_CLI_ARGS) {
            printError(); // too many arguments
            return;
        }

        cmdArgs[cmdArgsSize] = currentToken;
    }

    if (cmdArgsSize == 0) {
        printError(); // not enough arguments
        return;
    }

    // TODO: redirection and parallel commands
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

void runShellFrom(FILE *stream, bool printPrompt) {
    // includes the newline character
    char *line = NULL;
    size_t lineLength = 0;

    while (!atomic_exitting) {
        if (printPrompt) {
            printf("wish> ");
        }

        if (getline(&line, &lineLength, stream) == -1) {
            break;
        }

        removeNewline(line);
        interpretLine(line);
    }

    free(line);
}

int main(int argc, char *argv[]) {
    if (argc > 2) {
        exit(1);
    } else if (argc == 2) {
        // batch mode
        FILE *batchFile = fopen(argv[1], "r");
        if (batchFile == NULL) {
            printError();
            exit(1);
        }

        runShellFrom(batchFile, false);
    } else {
        // interactive mode
        runShellFrom(stdin, true);
    }

    return 0;
}

// TODO: running single command from path: `ls`
