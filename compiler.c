#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/wait.h>

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

void error_usage(char *path) {
    char msg[250];
    int len = sprintf(msg, "Usage: %s file [-o outfile]\n", path);
    write(2, msg, len);
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc != 2 && argc != 4) error_usage(argv[0]);

    int in_file_argv = -1;
    int out_file_argv = -1;
    if (argc == 4) {
        if (strcmp(argv[1], "-o") == 0) {
            in_file_argv = 3;
            out_file_argv = 2;
        } else if (strcmp(argv[2], "-o") == 0) {
            in_file_argv = 1;
            out_file_argv = 3;
        } else error_usage(argv[0]);
    }

    int fd = open(argv[in_file_argv], O_RDONLY);
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
                    case CELL_ADD:
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
                    case CELL_ADD:
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

    int fdout = open("./output.c", O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU);
    
    char buff[1000000];
    int len = sprintf(buff, "#include <stdio.h>\n#include <unistd.h>\nint main() {char array[30000];char *cell = array;");

    for (int i = 0; i < action_size; ++i) {
        switch (action[i].op) {
            case CELL_ADD:
                len += sprintf(buff+len, "*cell += %d;", action[i].value);
                break;
            case CELL_OFFSET:
                len += sprintf(buff+len, "cell += %d;", action[i].value);
                break;
            case JUMP:
                if (action[i].value > 0) {
                    len += sprintf(buff+len, "while(*cell) {");
                } else {
                    len += sprintf(buff+len, "}");
                }
                break;
            case INPUT:
                if (action[i].value != 0)
                    len += sprintf(buff+len, "cell += %d;", action[i].value);
                len += sprintf(buff+len, "read(1, cell, 1);");
                break;
            case OUTPUT:
                if (action[i].value != 0)
                    len += sprintf(buff+len, "*cell += %d;", action[i].value);
                len += sprintf(buff+len, "write(1, cell, 1);");
                break;
        }
    }
    free(action);

    len += sprintf(buff+len, "}\n");
    write(fdout, buff, len);
    close(fdout);

    int pid = fork();
    if (pid == 0) {
        if (out_file_argv == -1)
            execlp("gcc", "gcc", "-O3", "output.c", (char *)NULL);
        else {
            execlp("gcc", "gcc", "-O3", "-o", argv[out_file_argv], "output.c", (char *)NULL);
        }
    }
    else {
        waitpid(-1, NULL, 0);
        execlp("rm", "rm", "output.c", (char *)NULL);
    }
}