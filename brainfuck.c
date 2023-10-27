#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>

char array[30000];
char *cell = array;

void error_and_exit(const char *msg) {
    write(2, msg, strlen(msg));
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        char msg[250];
        sprintf(msg, "Usage: %s file\n", argv[0]);
        write(2, msg, strlen(msg));
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) error_and_exit("File non existen\n");

    char c;
    while (read(fd, &c, 1)) {
        switch (c) {
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
                read(0, &c, 1);
                *cell = c;
                break;
            }
            case '[': {
                if (!(*cell)) {
                    int left = 1;
                    while (left) {
                        read(fd, &c, 1);
                        switch (c) {
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
                }
                break;
            }
            case ']': {
                if (*cell) {
                    int right = 1;
                    while (right) {
                        lseek(fd, -2, SEEK_CUR);
                        read(fd, &c, 1);
                        switch (c) {
                            case '[': {
                                --right;
                                break;
                            }
                            case ']': {
                                ++right;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    return 0;
}