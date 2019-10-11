#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "context.h"
#include "errors.h"
#include "exec.h"
#include "utils.h"

char *parse_start(FILE *stream, int *linenum, EnvStack *stack, int nchars, ...);
exit_t parse_action(FILE *stream, int *linenum, EnvStack *stack);
void parse_assignment(FILE *stream, int *linenum, EnvStack *stack);
char *parse_capture(FILE *stream, EnvStack *stack);
char **parse_args(FILE *stream, char *command, int *linenum, EnvStack *stack);
char *parse_string(FILE *stream, int *linenum, EnvStack *stack);
char *parse_var(FILE *stream);

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
    push_stack(&stack, argv + 2);
    int linenum = 1;
    while(peek_char(stream) != EOF) {
        parse_start(stream, &linenum, &stack, 1, EOF);
    }

    fclose(stream);
    return 0;
}

char *parse_start(FILE *stream, int *linenum, EnvStack *stack, int nchars, ...) {
    char *result = NULL;
    char c;

    va_list ap;
    va_start(ap, nchars);
    char stop_chars[nchars];
    for (int i = 0; i < nchars; i++) stop_chars[i] = va_arg(ap, int);
    va_end(ap);

top:
    c = peek_char(stream);
    for (int j = 0; j < nchars; j++) {
        if (c == stop_chars[j]) {
            return result;
        }
    }
    switch(c) {
        case ' ':
        case '\t':
        case '\n':
            seek_for_spaces(stream, linenum);
            goto top;

        case '#':
            seek_onto_newline(stream, linenum);
            goto top;

        case 'a' ... 'z':
        case 'A' ... 'Z':
            parse_action(stream, linenum, stack);
            break;

        case '"':
            result = parse_string(stream, linenum, stack);
            break;

        //case '$':
            // Expand into args

        //case '(':
            // FIXME: What if multiple lines
            //break;

        //case '{':
            // FIXME: What if multiple lines

        //case '{':

        case ';':
        case EOF:
            break;

        default:
            die_invalid_syntax("Invalid syntax", *linenum);
    }
    return result;
}

exit_t parse_action(FILE *stream, int *linenum, EnvStack *stack) {
    char *name = NULL;
    char *value = NULL;
    char **argv;
    exit_t code = 0;

    char c = seek_until_consume_chars(stream, &name, 3, ' ', '=', '\n');
    do {
        switch(c) {
            case ' ':
                break;

            case '=':
                getc(stream);  // Comsume '"'
                value = parse_start(stream, linenum, stack, 2, '\n', ';');
                if (value) add_stack_var(stack, name, value);
                goto finish;

            default:
                argv = parse_args(stream, name, linenum, stack);

                push_stack(stack, argv);
                // FIXME: Figure out how to return exit code and result
                code = run(name, stack);
                set_last_exit_code(stack, code);
                pop_stack(stack);
                goto finish;
        }
    } while((c = peek_char(stream)) != EOF);

finish:
    if (name) free(name);
    if (value) free(value);
    return code;
}

char *parse_capture(FILE *stream, EnvStack *stack) {
    return "";
}

char **parse_args(FILE *stream, char *command, int *linenum, EnvStack *stack) {
    long conf_max_args = sysconf(_SC_ARG_MAX);
    size_t max_args = 64;
    if (conf_max_args > 0) max_args = (size_t) conf_max_args;
    char **argv_buf = must_malloc(sizeof *argv_buf * max_args);
    argv_buf[0] = strdup(command);
    size_t argc = 1;

    char* arg;
    // FIXME: Using parse_start causes things without quotes to run as commands
    while((arg = parse_start(stream, linenum, stack, 3, ';', '\n', ' ')) != NULL) {
        //printf("arg: [%s] ", arg);
        if (argc >= max_args) die_invalid_syntax("Too many args", *linenum);
        argv_buf[argc++] = arg;
    }

    argv_buf[argc] = NULL;
    argv_buf = must_realloc(argv_buf, sizeof(*argv_buf) * (argc + 1));
    return argv_buf;
}

char *parse_string(FILE *stream, int *linenum, EnvStack *stack) {
    StrBuilder *build = str_build_create();
    assert(getc(stream) == '"');

    char c;
    while((c = getc(stream)) != EOF) {
        switch(c) {
            case '$':
                str_build_add_str(build, parse_var(stream));

            case '\\':
                c = getc(stream);
                switch(c) {
                    case 'n': c = '\n'; break;
                    case 't': c = '\t'; break;
                    case '\\': c = '\\'; break;
                    case '"': c = '"'; break;
                    case '*': c = '*'; break;
                    case '$': c = '$'; break;
                }
                if (c != EOF) str_build_add_c(build, c);
                break;

            case '\n':
                *linenum = *linenum + 1;

            case '"':
                goto finish;

            default:
                str_build_add_c(build, c);
        }
    }

finish:
    return str_build_to_str(build);
}

char *parse_var(FILE *stream) {
    return ""; // FIXME!
}

char *expand_var(char *var, EnvStack *stack) {
    return var;
}
