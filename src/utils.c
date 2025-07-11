#include "utils.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

char *slurp_file_str(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror(filename);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    size_t file_size = (size_t)ftell(file);
    fseek(file, 0, SEEK_SET);

    /* +1 for null-terminator */
    char *buffer = malloc(file_size + 1);
    if (!buffer) {
        perror(filename);
        fclose(file);
        return NULL;
    }

    if (fread(buffer, 1, file_size, file) != file_size) {
        perror(filename);
        free(buffer);
        fclose(file);
        return NULL;
    }

    buffer[file_size] = '\0';
    fclose(file);
    return buffer;
}
