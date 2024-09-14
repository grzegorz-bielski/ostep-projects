#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

void decompressFile(char *fileName) {
    FILE *file = fopen(fileName, "r");
    if (file == NULL) {
        printf("wunzip: cannot open file\n");
        exit(1);
    }

    char inBuffer[sizeof(uint32_t) + sizeof(char)]; // (usually) 4 byte int in binary + 1 ACII char
    size_t itemsToRead = sizeof(inBuffer) / sizeof((inBuffer)[0]);
    size_t itemSize = sizeof((inBuffer)[0]);
    size_t chunksRead = 0;

    while((chunksRead = fread(inBuffer, itemSize, itemsToRead, file)) != 0) {
        if (chunksRead < 5) {
            printf("wunzip: incomplete data\n");
            exit(1);
        }

        // RLE decompression
        // see EncodedData in wzip/main.c for more details
    
        char character = inBuffer[sizeof(uint32_t)];
        uint32_t count;
        memcpy(&count, inBuffer, sizeof(uint32_t));

        // could use `arpa/inet.h` / `ntohl` to convert to host byte order for consistency,
        // but it breaks test cases :shrug:
        // count = ntohl(count); 

        for (int i = 0; i < count; i++) {
            printf("%c", character); 
        }
    }

    if (fclose(file)) {
        perror("wunzip: cannot close file\n");
        exit(1);
    }
}

int main(int argc, char *argv[]) {
    if (argc <= 1) {
        printf("wunzip: file1 [file2 ...]\n");
        exit(1);
    }

    for (int i = 1; i < argc; i++) {
        decompressFile(argv[i]);
    }

    return 0;
}
