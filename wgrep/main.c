#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int isEmptyString(const char *str) {
    return str[0] == '\0';
}

void searchStream(FILE *file, char *searchTerm) {
    char *currentLine = NULL;
    size_t currentLineLength = 0;
    size_t charsReadOrError;
    
    // getLine handles arbitrary length lines
    while ((charsReadOrError = getline(&currentLine, &currentLineLength, file) != -1)) {
        if (strstr(currentLine, searchTerm) != NULL) {
            printf("%s", currentLine);
        }
    }

    // always free the memory allocated by getline
    free(currentLine);

    // getline returns -1 when it reaches the end of the file or encounters an error
    if (charsReadOrError == -1 && !feof(file)) {
        perror("wgrep: cannot read file\n");
        exit(1);
    }

    if (fclose(file)) {
        perror("wgrep: cannot close file\n");
        exit(1);
    }
}

void searchFile(char *fileName, char *searchTerm) {
    FILE *file = fopen(fileName, "r");
    if (file == NULL) {
        printf("wgrep: cannot open file\n");
        exit(1);
    }

    searchStream(file, searchTerm);
}

void searchStdin(char *searchTerm) {
    // stdin is a FILE pointer that is already open
    searchStream(stdin, searchTerm);
}

int main(int argc, char *argv[]) {
    if (argc <= 1) {
        printf("wgrep: searchterm [file ...]\n");
        exit(1);
    }

    char *searchTerm = argv[1];

    if (isEmptyString(searchTerm)) {
        exit(0);
    }

    if (argc == 2) {
        searchStdin(searchTerm);
    } else {
        for (int i = 2; i < argc; i++) {
            char *fileName = argv[i];
            searchFile(fileName, searchTerm);
        }
    }

    return 0;
}
