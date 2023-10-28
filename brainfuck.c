#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        char msg[250];
        sprintf(msg, "Usage: %s file\n", argv[0]);
        write(2, msg, strlen(msg));
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        char msg[] = "File non existent\n";
        write(2, msg, strlen(msg));
        return 1;
    }

    int file_size = lseek(fd, 0, SEEK_END);
    char *file = malloc(file_size);
    lseek(fd, 0, SEEK_SET);
    read(fd, file, file_size);

    // Precompile jumps
    int jump[file_size]; // a much better solution would be an unordered map
    for (int i = 0; i < file_size; ++i) {
        // a better algorithm would be to have a running stack
        if (file[i] == '[') {
            int left = 1;
            int j = i;
            while (left) {
                switch (file[++j]) {
                    case '[': {
                        ++left;
                        break;
                    }
                    case ']': {
                        --left;
                        break;
                    }
                }
            }
            jump[i] = j - i;
            jump[j] = i - j;
        } else if (file[i] != ']') {
            jump[i] = 0;
        }
    }

    char array[30000]; // initialized at 0 by the compiler
    char *cell = array;

    int index = 0;
    while (index < file_size) {
        switch (file[index]) {
            case '>': {
                ++cell;
                break;
            }
            case '<': {
                --cell;
                break;
            }
            case '+': {
                ++(*cell);
                break;
            }
            case '-': {
                --(*cell);
                break;
            }
            case '.': {
                write(1, cell, 1);
                break;
            }
            case ',': {
                if (!read(0, cell, 1)) return 0;
                break;
            }
            case '[': {
                if (!(*cell)) index += jump[index];
                break;
            }
            case ']': {
                if (*cell) index += jump[index]; 
                break;
            }
        }
        ++index;
    }

    free(file);
    return 0;
}