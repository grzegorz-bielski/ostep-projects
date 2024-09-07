#include <stdio.h>
#include <stdlib.h>

#define MAX_LINE_LENGTH 1000

void printFile(char *fileName) {
    FILE *file = fopen(fileName, "r");
    if (file == NULL) {
        printf("wcat: cannot open file\n");
        exit(1);
    }

    char lineBuffer[MAX_LINE_LENGTH];

    // fgets() reads a line from the file and stores it in lineBuffer
    // fgets() returns NULL when it reaches the end of the file or encounters an error
    // fgets() reads at most 999 characters from the file in a single call
    // if line is longer than 999 characters, fgets() reads the first 999 characters and appends a null character at the end
    while (fgets(lineBuffer, MAX_LINE_LENGTH, file) != NULL) {
        printf("%s", lineBuffer);
    }

    if (fclose(file)) {
        perror("wcat: cannot close file\n");
        exit(1);
    }
}


int main(int argc, char *argv[]) {
    if (argc <= 1) {
        // below info is not allowed by test suite
        // printf("wcat: file1 file2 ...\n");
        exit(0);
    }

    for (int i = 1; i < argc; i++) {
        char *fileName = argv[i];
        printFile(fileName);
    }

    return 0;
}
