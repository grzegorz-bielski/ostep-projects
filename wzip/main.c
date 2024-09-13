// run-length encoding (RLE):
// n chars (run length) of the same type in a row -> nChar: aaaaaaaaaabbbb -> 10a4b

// compressed data format: 4 byte int in binary + 1 ACII char

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TODO: https://github.com/remzi-arpacidusseau/ostep-projects/tree/master/initial-utilities#wzip-and-wunzip

#define MAX_OUT_BUFFER_SIZE 100
// struct

typedef struct {
    int count;
    char character;
} EncodedData;

// TODO: write to stdout
void writeToStream(EncodedData outBuffer[MAX_OUT_BUFFER_SIZE], size_t latestOutBufferIndex) {
    // printf("writeToStream: %ld\n", latestOutBufferIndex);
    // printf("Size of int: %zu bytes\n", sizeof(int)); // 4 bytes


    for (size_t i = 0; i <= latestOutBufferIndex; i++) {
        EncodedData current = outBuffer[i];
        // fprintf(stdout, "%d%c", current.count, current.character);

         fwrite(&current.count, sizeof(int), 1, stdout);
         fwrite(&current.character, sizeof(char), 1, stdout);
    }

    // fflush(stdout);
}

int main(int argc, char *argv[]) {
    if (argc <= 1) {
        printf("wzip: file\n");
        exit(1);
    }

    char *fileName = argv[1];
    FILE *file = fopen(fileName, "r");
    if (file == NULL) {
        printf("wgrep: cannot open file\n");
        exit(1);
    }

    char inBuffer[10]; // TODO: increase buffer size
    size_t itemsToRead = sizeof(inBuffer) / sizeof((inBuffer)[0]);
    size_t itemSize = sizeof((inBuffer)[0]);
    size_t chunksRead = 0;

    EncodedData outBuffer[MAX_OUT_BUFFER_SIZE];
    size_t latestOutBufferIndex = -1;

    while((chunksRead = fread(inBuffer, itemSize, itemsToRead, file)) != 0) {
        for (size_t i = 0; i < chunksRead; i++) {
            char currentChar = inBuffer[i];
            // initial character
            if (latestOutBufferIndex == -1) {
                EncodedData data = { .count = 1, .character = currentChar };
                outBuffer[++latestOutBufferIndex] = data;
            } 
            // same character
            else if (outBuffer[latestOutBufferIndex].character == currentChar) {
                outBuffer[latestOutBufferIndex].count++;
            } 
            // character changed
            else {
                // write to outBuffer
                EncodedData data = {.count = 1, .character = currentChar };
                outBuffer[++latestOutBufferIndex] = data;

                // check if it's full and write to file
                if (latestOutBufferIndex >= (MAX_OUT_BUFFER_SIZE - 1)) {
                    writeToStream(outBuffer, latestOutBufferIndex);

                    // reset outBuffer
                    latestOutBufferIndex = 0;
                    memset(outBuffer, 0, sizeof(outBuffer));
                }
            }
        }
    }

    // // print outBuffer values in a loop
    // for (size_t i = 0; i <= latestOutBufferIndex; i++) {
    //     EncodedData current = outBuffer[i];
    //     printf("%d%c\n", current.count, current.character);
    // }

    writeToStream(outBuffer, latestOutBufferIndex);

    if (fclose(file)) {
        perror("wgrep: cannot close file\n");
        exit(1);
    }

    return 0;
}
