#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
// #include <stdint.h>
#include <stdbool.h>
#include <ctype.h>

// https://github.com/remzi-arpacidusseau/ostep-projects/tree/master/processes-shell

// max amount of arguments in a single command
#define MAX_CLI_ARGS 1024
// amount of paths in the path array
#define MAX_PATH_SIZE 512
// max length of a single path
#define MAX_PATH_LENGTH 4096
// max amount of parallel commands using `&`
#define MAX_PARALLEL_COMMANDS 1024

// not a built-in command code
#define NOT_A_BUILTIN -4
#define CMD_NO_OP -3
#define CMD_RUN -2
#define CMD_RUN_FAILURE -1

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
        size_t cmdPathSize = strlen(oldPathElem) + cmdSize + 2; // +1 for null terminator, +1 for '/'
        cmdPath = (char *)malloc(cmdPathSize); // freed in the tryRunningCommand
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


// returns `CMD_RUN_FAILURE` or child pid
int tryRunningCommand(char *cmdArgs[], size_t cmdArgsSize, char* redirectTo) { 
    char *cmdPath = getCommandPath(cmdArgs[0]);
    if (cmdPath == NULL) {
        printError();
        return CMD_RUN_FAILURE;
    }

    int rc = fork();
    if (rc < 0) {
        // fork failed, no child process is created
        printError();

        free(cmdPath);

        return CMD_RUN_FAILURE;
    } else if (rc == 0) {
        // child: execute the command

        // handle redirection
        if (redirectTo != NULL) {
            // close stdout
            close(STDOUT_FILENO);
            // `open` will use first available file descriptor, starting from 0
            // STDOUT_FILENO (1) will be the first available file descriptor
            // so all output that writes to the `STDOUT_FILENO` will go to the `redirectTo` file
            open(redirectTo, O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);

            // close stderr
            close(STDERR_FILENO);
            // redirect stderr to the same file as stdout, same as above and `2>&1`
            open(redirectTo, O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);
        }

        // update execArgs
        // command name without path, specifying path is not supported
        char *execArgs[cmdArgsSize + 1];
        execArgs[0] = cmdArgs[0];
        for (size_t i = 1; i < cmdArgsSize; i++) {
            execArgs[i] = cmdArgs[i];
        }
        execArgs[cmdArgsSize] = NULL;

        if (execv(cmdPath, execArgs)) {
            printError();
        }

        // never reached
        return CMD_RUN;

    } else {
        // parent: wait for the child to finish in the caller
        
        free(cmdPath);

        return rc; // return the child pid
    }
}

// modifies global `atomic_exitting` (!)
int handleExit(char *cmdArgs[], size_t cmdArgsSize) {
    if (cmdArgsSize != 1) {
        // too many arguments for exit
        printError();
        return CMD_RUN;
    }

    atomic_exitting = true;
    return CMD_RUN;
}

int handleCd(char *cmdArgs[], size_t cmdArgsSize) {
    if (cmdArgsSize != 2) {
        // too many arguments for cd
        printError();
        return CMD_RUN;
    }

    if (chdir(cmdArgs[1]) != 0) {
        printError();
    }

    return CMD_RUN;
}

// modifies global `path` and `pathSize` (!)
int handlePath(char *cmdArgs[], size_t cmdArgsSize) {
    if (cmdArgsSize > MAX_PATH_SIZE) {
        // too many arguments for path
        printError();
        return CMD_RUN;
    }

    // clear path
    pathSize = 0;
    memset(path, 0, sizeof(path));

    for (size_t i = 1; i < cmdArgsSize; i++) {
        char *absolutePath = (char *) malloc(MAX_PATH_LENGTH);
        // TODO: when to free the memory?

        if (realpath(cmdArgs[i], absolutePath) == NULL) {
            printError(); // invalid path
            return CMD_RUN;
        }

        path[i - 1] = absolutePath;
    }

    pathSize = cmdArgsSize - 1;

    return CMD_RUN;
}

int tryRunningBuiltIns(char *cmdArgs[], size_t cmdArgsSize) {
    if (strcmp(cmdArgs[0], "exit") == 0) {
        return handleExit(cmdArgs, cmdArgsSize);
    }
    
    if (strcmp(cmdArgs[0], "cd") == 0) {
        return handleCd(cmdArgs, cmdArgsSize);
    } 
    
    if (strcmp(cmdArgs[0], "path") == 0) {
        return handlePath(cmdArgs, cmdArgsSize);
    }

    return NOT_A_BUILTIN;
}

void removeNewline(char *str) {
    // find the position of the \n
    size_t len = strcspn(str, "\n");
    if (str[len] == '\n') {
        str[len] = '\0';
    }
}

void trimWhitespace(char *str) {
    char *start = str;
    char *end;

    // Trim leading space
    while (isspace((unsigned char)*start)) {
        start++;
    }

    // Is it all whitespace?
    if (*start == 0) {
        *str = 0;
        return;
    }

    // Trim trailing space
    end = start + strlen(start) - 1; // points to the last character before null terminator
    while (end > start && isspace((unsigned char)*end)) {
        end--; // move backwards from the null terminator
    }

    // Write new null terminator
    *(end + 1) = '\0';

    // Shift the trimmed string to the beginning
    memmove(str, start, end - start + 2); // `end - start + 1` is the length of the string, +1 for the null terminator
}

// returns `CMD_RUN_FAILURE`, `CMD_RUN`, `CMD_NO_OP` or child pid
int interpretSubCmd(char *subCmd) {
    size_t cmdArgsSize = 0;
    char *cmdArgs[MAX_CLI_ARGS];

    trimWhitespace(subCmd);

    if (strlen(subCmd) == 0) {
        // skip empty lines
        return CMD_NO_OP;
    }

    // handle redirection
    // only following format is supported: `cmd args > file` i.e `lhs > rhs`
    char* lhs = strsep(&subCmd, ">");
    trimWhitespace(lhs); // TODO: might be unnecessary
    if (strlen(lhs) == 0) {
        printError(); // no command
        return CMD_RUN_FAILURE;
    }

    char* rhs = subCmd;
    if (rhs != NULL) {
        rhs = strdup(rhs);
        trimWhitespace(rhs); // TODO: might be unnecessary

        // output file validation
        if (strlen(rhs) == 0) {
            printError(); // no output file
            return CMD_RUN_FAILURE;
        }

        // check if there are more than one arguments for output file
        // if there are: tempToken will be the first argument, rhs will be the second argument
        char* tempToken = strsep(&rhs, " ");

        if (rhs != NULL) {
            printError(); // too many arguments for output file
            return CMD_RUN_FAILURE;
        }

        rhs = tempToken;
    }

    // TODO: build struct for cmd exec input (?)
    // TODO: add another, outer loop for parallel commands

    for (char *currentToken;(currentToken = strsep(&lhs, " ")) != NULL; cmdArgsSize++) {
        if (cmdArgsSize >= MAX_CLI_ARGS) {
            printError(); // too many arguments
            return CMD_RUN_FAILURE;
        }

        cmdArgs[cmdArgsSize] = currentToken;
    }

    if (cmdArgsSize == 0) {
        printError(); // not enough arguments
        return CMD_RUN_FAILURE;
    }

    int buildIntRes = tryRunningBuiltIns(cmdArgs, cmdArgsSize);

    if (buildIntRes == NOT_A_BUILTIN) {
        return tryRunningCommand(cmdArgs, cmdArgsSize, rhs);
    } else {
        return buildIntRes;
    }
}

void interpretLine(char *line) {
    size_t subCmdSize = 0;
    char *sumCmds[MAX_PARALLEL_COMMANDS];

    removeNewline(line);

    // split by `&`
    char *currentToken;
    while((currentToken = strsep(&line, "&")) != NULL) {
        if (subCmdSize >= MAX_PARALLEL_COMMANDS) {
            printError(); // too many arguments
            return;
        }

        trimWhitespace(currentToken);

        if (strlen(currentToken) == 0) {
            continue;; // skip singular & without error
        }

        sumCmds[subCmdSize++] = currentToken;
    }

    // run commands
    pid_t pids[MAX_PARALLEL_COMMANDS];
    // not every subCmd is a forked process
    int numOfForkedProcesses = 0;
    for (size_t i = 0; i < subCmdSize; i++) {
        int res = interpretSubCmd(sumCmds[i]);

        if (
            res != CMD_RUN_FAILURE && // won't wait stop for failed commands
            res != CMD_RUN && 
            res != CMD_NO_OP
        ) {
            // res is a child pid
            pids[numOfForkedProcesses++] = res;
        }
    }

    // wait for all forked processes
    for (size_t i = 0; i < numOfForkedProcesses; i++) {
        if (waitpid(pids[i], NULL, 0) == -1) {
            printError();
            return;
        }
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

        interpretLine(line);
    }

    free(line);
}

int main(int argc, char *argv[]) {
    if (argc > 2) {
        printError();
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
