// run-length encoding compression alg (RLE):
// n chars (run length) of the same type in a row -> nChar: aaaaaaaaaabbbb -> 10a4b
// compressed data format: 4 byte int in binary + 1 ACII char

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// for passing 6th test on a single buffer
// #define MAX_OUT_BUFFER_SIZE 512 * 2024

// min size for streaming like functionality: 
// keeps current and the previous values only
// could be increased for better I/O performance
#define MAX_OUT_BUFFER_SIZE 2

typedef struct {
    int count;
    char character;
} EncodedData;

void writeToStream(EncodedData outBuffer[], size_t outBufferIndex) {
    for (size_t i = 0; i <= outBufferIndex; i++) {
        EncodedData current = outBuffer[i];

        // could use `arpa/inet.h` / `htonl` to convert to network byte order for consistency, 
        // but it breaks the test cases

        fwrite(&current.count, sizeof(int), 1, stdout);
        fwrite(&current.character, sizeof(char), 1, stdout);
    }
}

void compressFile(char *fileName, EncodedData outBuffer[], size_t *outBufferIndex) {
     FILE *file = fopen(fileName, "r");
    if (file == NULL) {
        printf("wgrep: cannot open file\n");
        exit(1);
    }

    char inBuffer[128]; // arbitrary buffer size
    size_t itemsToRead = sizeof(inBuffer) / sizeof((inBuffer)[0]);
    size_t itemSize = sizeof((inBuffer)[0]);
    size_t chunksRead = 0;

    while((chunksRead = fread(inBuffer, itemSize, itemsToRead, file)) != 0) {
        for (size_t i = 0; i < chunksRead; i++) {
            char currentChar = inBuffer[i];

            if (
                // initial character
                ((*outBufferIndex) == -1) || 
                // character changed
                (outBuffer[*outBufferIndex].character != currentChar)
            ) {
                // check if buffer will become full with next character and write it to the stream if so
                if ((*outBufferIndex + 1) >= (MAX_OUT_BUFFER_SIZE - 1)) {
                    writeToStream(outBuffer, *outBufferIndex);

                    // reset outBuffer
                    memset(outBuffer, 0, sizeof(EncodedData) * MAX_OUT_BUFFER_SIZE);
                    *outBufferIndex = -1;
                }

                // add new character to the buffer
                EncodedData data = { .count = 1, .character = currentChar };
                outBuffer[++(*outBufferIndex)] = data;

            } 
            // same character
            else if (outBuffer[*outBufferIndex].character == currentChar) {
                outBuffer[*outBufferIndex].count++;
            }
        }
    }

    if (fclose(file)) {
        perror("wgrep: cannot close file\n");
        exit(1);
    }
}

int main(int argc, char *argv[]) {
    if (argc <= 1) {
        printf("wzip: file1 [file2 ...]\n");
        exit(1);
    }

    EncodedData outBuffer[MAX_OUT_BUFFER_SIZE];
    size_t outBufferIndex = -1;

    for (int i = 1; i < argc; i++) {
        compressFile(argv[i], outBuffer, &outBufferIndex);
    }

    writeToStream(outBuffer, outBufferIndex);

    return 0;
}
