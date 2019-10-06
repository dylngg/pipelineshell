#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "errors.h"
#include "context.h"
#include "exec.h"

void parse_start(FILE *stream, char c, int *linenum, EnvStack *stack);
void parse_action(FILE *stream, int *linenum, EnvStack *stack);
void parse_assignment(FILE *stream, int *linenum, EnvStack *stack);
int parse_command(FILE *stream, int *linenum, EnvStack *stack);
void parse_args(FILE *stream, char *command, EnvStack *stack);
char *parse_string(char *string);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "No filename given\n");
        exit(1);
    }
    char *filename = argv[1];

    FILE *stream = fopen(filename, "r");
    if (!stream) {
        perror("File could not be read");
        return 1;
    }

    EnvStack stack = {0};
    push_stack(&stack, argv++);
    update_last_exit_code(&stack, 0);
    int linenum = 1;
    char c = getc(stream);
    do {
        parse_start(stream, c, &linenum, &stack);
    } while((c = getc(stream)) != EOF);

    fclose(stream);
    return 0;
}

void parse_start(FILE *stream, char c, int *linenum, EnvStack *stack) {
    if (isalpha(c)) {
        parse_action(stream, linenum, stack);
        return;
    }

    switch(c) {
        case '\n':
            (*linenum)++;
        case ';':
            break;

        case '#':
            seek_onto_newline(stream);
            (*linenum)++;
            break;

        case ' ':
            seek_spaces(stream);
            break;

        //case '"':
            // FIXME: What if multiple lines
            //break;

        //case '(':
            // FIXME: What if multiple lines
            //break;

        //case '{':
            // FIXME: What if multiple lines

        //case '{':

        default:
            die_invalid_syntax("Invalid start character", *linenum);
    }
    return;
}

void parse_action(FILE *stream, int *linenum, EnvStack *stack) {
    if (contains_before_chars(stream, '=', 3, ' ', '\t', '\n')) {
        parse_assignment(stream, linenum, stack);
    }
    parse_command(stream, linenum, stack);
}

void parse_assignment(FILE *stream, int *linenum, EnvStack *stack) {
    exit(1);
}

int parse_command(FILE *stream, int *linenum, EnvStack *stack) {
    char *command = NULL;
    char last = seek_onto_chars(stream, &command, 3, ' ', '\t', '\n');
    if (last != '\n') {
        parse_args(stream, command, stack);
    }
    else {
        char** argv = malloc(sizeof *argv * 2);
        if (!argv) die_no_mem();
        argv[0] = strdup(command);
        argv[1] = NULL;
        if (!argv[0]) die_no_mem();
        push_stack(stack, argv);
    }

    int r = run(command, stack);
    pop_stack(stack);
    free(command);
    return r;
}

void parse_args(FILE *stream, char *command, EnvStack *stack) {
    size_t bufsize = 32;
    char **argv_buf = malloc(sizeof *argv_buf * bufsize);
    if (!argv_buf) die_no_mem();
    argv_buf[0] = strdup(command);
    size_t curr_size = 1;

    char last;
    char *arg = "";
    do {
        last = seek_onto_chars(stream, &arg, 3, ' ', '\t', '\n');
        if (arg[0] == '"') arg = parse_string(arg);
        argv_buf[curr_size++] = arg;
        if (last == EOF || last == '\n') break;

        // + 1 for null terminator
        if (curr_size + 1 >= bufsize) {
            bufsize *= 2;
            // FIXME: Non-GNU will not free new alloc
            argv_buf = realloc(argv_buf, bufsize);
            if (!argv_buf) die_no_mem();
        }
    } while(true);

    // Free any memory we don't need.
    argv_buf[curr_size] = NULL;
    argv_buf = realloc(argv_buf, sizeof(*argv_buf) * (curr_size + 1));
    if (!argv_buf) die_no_mem();
    push_stack(stack, argv_buf);
}

char *parse_string(char *string) {
    return strdup(string);
}
