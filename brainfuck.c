#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <assert.h>

typedef enum {
    CELL_OFFSET,
    CELL_ADD,
    JUMP,
    INPUT,
    OUTPUT,
    NOOP
} Operation;

typedef struct Action {
    Operation op;
    int value;
} Action;

typedef struct Element Element;

struct Element {
    int value;
    Element *prev;
};

Element *top = NULL;

void push(int x) {
    Element *e = malloc(sizeof(Element));
    e->prev = top;
    e->value = x;
    top = e;
}

int pop() {
    int x = top->value;
    Element *tmp = top;
    top = tmp->prev;
    free(tmp);
    return x;
}

int main(int argc, char *argv[]) {
/*     argc = 2;
    argv[0] = "./brainfuck";
    argv[1] = "sample.inp"; */
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
    close(fd);


    // 1st pass: Calculate # of actions
    int action_size = 0;
    Operation state = NOOP;
    for (int i = 0; i < file_size; ++i) {
        switch (file[i]) {
            case '<':
            case '>':
                switch (state) {
                    case NOOP:
                        state = CELL_OFFSET;
                        break;
                    case CELL_OFFSET:
                        break;
                    default:
                        ++action_size;
                        state = CELL_OFFSET;
                        break;
                }
                break;
            case '-':
            case '+':
                switch (state) {
                    case NOOP:
                        state = CELL_ADD;
                        break;
                    case CELL_ADD:
                        break;
                    default:
                        ++action_size;
                        state = CELL_ADD;
                        break;
                }
                break;
            case '[':
            case ']':
                switch (state) {
                    case NOOP:
                        state = JUMP;
                        break;
                    default:
                        ++action_size;
                        state = JUMP;
                        break;
                }
                break;
            case ',':
                switch (state) {
                    case NOOP:
                    case CELL_OFFSET:
                        state = INPUT;
                        break;
                    default:
                        ++action_size;
                        state = INPUT;
                        break;
                }
                break;
            case '.':
                switch (state) {
                    case NOOP:
                    case CELL_OFFSET:
                        state = OUTPUT;
                        break;
                    default:
                        ++action_size;
                        state = OUTPUT;
                        break;
                }
                break;
        }
    }
    ++action_size;

    // 2nd pass: Compile actions
    Action *action = malloc(action_size*sizeof(Action));
    Action *curr = action;
    curr->op = NOOP;
    for (int i = 0; i < file_size; ++i) {
        switch (file[i]) {
            case '>':
                switch (curr->op) {
                    case NOOP:
                        curr->op = CELL_OFFSET;
                    case CELL_OFFSET:
                        ++curr->value;
                        break;
                    default:
                        ++curr;
                        curr->op = CELL_OFFSET;
                        curr->value = 1;
                        break;
                }
                break;
            case '<':
                switch (curr->op) {
                    case NOOP:
                        curr->op = CELL_OFFSET;
                    case CELL_OFFSET:
                        --curr->value;
                        break;
                    default:
                        ++curr;
                        curr->op = CELL_OFFSET;
                        curr->value = -1;
                        break;
                }
                break;
            case '+':
                switch (curr->op) {
                    case NOOP:
                        curr->op = CELL_ADD;
                    case CELL_ADD:
                        ++curr->value;
                        break;
                    default:
                        ++curr;
                        curr->op = CELL_ADD;
                        curr->value = 1;
                        break;
                }
                break;
            case '-':
                switch (curr->op) {
                    case NOOP:
                        curr->op = CELL_ADD;
                    case CELL_ADD:
                        --curr->value;
                        break;
                    default:
                        ++curr;
                        curr->op = CELL_ADD;
                        curr->value = -1;
                        break;
                }
                break;
            case '[':
                switch (curr->op) {
                    case NOOP:
                        curr->op = JUMP;
                        break;
                    default:
                        ++curr;
                        curr->op = JUMP;
                        break;
                }
                push(curr - action);
                break;
            case ']':
                switch (curr->op) {
                    case NOOP:
                        curr->op = JUMP;
                        break;
                    default:
                        ++curr;
                        curr->op = JUMP;
                        break;
                }
                int left = pop();
                int right = curr - action;
                action[left].value = right - left;
                action[right].value = left - right;
                break;
            case ',':
                switch (curr->op) {
                    case NOOP:
                        curr->op = INPUT;
                        curr->value = 0;
                        break;
                    case CELL_OFFSET:
                        curr->op = INPUT;
                        break;
                    default:
                        ++curr;
                        curr->op = INPUT;
                        curr->value = 0;
                        break;
                }
                break;
            case '.':
                switch (curr->op) {
                    case NOOP:
                        curr->op = OUTPUT;
                        curr->value = 0;
                        break;
                    case CELL_OFFSET:
                        curr->op = OUTPUT;
                        break;
                    default:
                        ++curr;
                        curr->op = OUTPUT;
                        curr->value = 0;
                        break;
                }
                break;
        }
    }
    free(file);

    char array[30000]; // initialized at 0 by the compiler
    char *cell = array;

    int index = 0;
    while (index < action_size) {
        switch (action[index].op) {
            case CELL_ADD:
                *cell += action[index].value;
                break;
            case CELL_OFFSET:
                cell += action[index].value;
                break;
            case JUMP:
                if ((action[index].value < 0 && *cell) || (action[index].value > 0 && !(*cell)))
                    index += action[index].value;
                break;
            case INPUT:
                cell += action[index].value;
                read(1, cell, 1);
                break;
            case OUTPUT:
                cell += action[index].value;
                write(1, cell, 1);
                break;
        }
        ++index;
    }
    return 0;
}