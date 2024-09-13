// run-length encoding (RLE):
// n chars (run length) of the same type in a row -> nChar: aaaaaaaaaabbbb -> 10a4b

// compressed data format: 4 byte int in binary + 1 ACII char

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_OUT_BUFFER_SIZE 64 * 1024

typedef struct {
    int count;
    char character;
} EncodedData;

void writeToStream(EncodedData outBuffer[], size_t *outBufferIndex) {
    for (size_t i = 0; i <= (*outBufferIndex); i++) {
        EncodedData current = outBuffer[i];

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
                EncodedData data = { .count = 1, .character = currentChar };
                outBuffer[++(*outBufferIndex)] = data;
            } else if (outBuffer[*outBufferIndex].character == currentChar) {
                outBuffer[*outBufferIndex].count++;
            }

            // escape hatch: flush outBuffer if it's full
            if ((*outBufferIndex) >= (MAX_OUT_BUFFER_SIZE - 1)) {
                writeToStream(outBuffer, outBufferIndex);

                // reset outBuffer
                *outBufferIndex = -1;
                memset(outBuffer, 0, sizeof(EncodedData) * MAX_OUT_BUFFER_SIZE);
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

    writeToStream(outBuffer, &outBufferIndex);

    return 0;
}
