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

Result *parse_scope(FILE *stream, char *argv[], int *linenum, EnvStack *stack, int nchars, ...);
Result *parse_start(FILE *stream, int *linenum, EnvStack *stack, int nchars, ...);
Result *parse_action(FILE *stream, int *linenum, EnvStack *stack);
Result *parse_assignment(FILE *stream, char *name, int *linenum, EnvStack *stack);
Result *parse_command(FILE *stream, char *name, int *linenum, EnvStack *stack);
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
    int linenum = 1;
    // When we pop the stack, we free all the resources; argv is not allocated
    char **argv_copy = copy_argv(argv, argc);
    Result *result = parse_scope(stream, argv_copy, &linenum, &stack, 0);
    exit_t code = result->code;
    destroy_result(result);

    fclose(stream);
    return code;
}

Result *parse_scope(FILE *stream, char *argv[], int *linenum, EnvStack *stack, int nchars, ...) {
    if (argv) push_stack(stack, argv);
    else push_stack_from_prev(stack);

    va_list ap;
    va_start(ap, nchars);
    char stop_chars[nchars];
    for (int i = 0; i < nchars; i++) stop_chars[i] = va_arg(ap, int);
    va_end(ap);

    Result *result = NULL;
    char c;
    while((c = peek_char(stream)) != EOF) {
        if (result) free(result);  // Free previous result
        for (int j = 0; j < nchars; j++) if (c == stop_chars[j]) goto finish;
        result = parse_start(stream, linenum, stack, 1, EOF);
    }

finish:
    pop_stack(stack);
    return result ? result : create_empty_result();
}

Result *parse_start(FILE *stream, int *linenum, EnvStack *stack, int nchars, ...) {
    Result *result = NULL;
    char *tmp = "";
    char c;

    va_list ap;
    va_start(ap, nchars);
    char stop_chars[nchars];
    for (int i = 0; i < nchars; i++) stop_chars[i] = va_arg(ap, int);
    va_end(ap);

top:
    c = peek_char(stream);
    for (int j = 0; j < nchars; j++) if (c == stop_chars[j]) goto finish;
    switch(c) {
        case ' ':
        case '\t':
        case '\n':
            seek_for_whitespace(stream, linenum);
            goto top;

        case '#':
            seek_onto_newline(stream, linenum);
            goto top;

        case 'a' ... 'z':
        case 'A' ... 'Z':
            result = parse_action(stream, linenum, stack);
            break;

        case '"':
            tmp = parse_string(stream, linenum, stack);
            result = create_result(tmp);
            free(tmp);
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
            getc(stream);
            break;

        case EOF:
            break;

        default:
            seek_until_chars(stream, &tmp, 4, ' ', '\t', '\n', ';');
            result = create_result(tmp);
            free(tmp);
            break;
    }
finish:
    return result;
}

Result *parse_action(FILE *stream, int *linenum, EnvStack *stack) {
    char *name = NULL;

    char c = seek_until_chars(stream, &name, 5, ' ', '\t', '=', ';', '\n');
    if (c == ' ' || c == '\t') c = seek_for_spaces(stream);

    if (c == '=')
        // Is a assignment
        return parse_assignment(stream, name, linenum, stack);

    // Is a command
    return parse_command(stream, name, linenum, stack);
}

Result *parse_assignment(FILE *stream, char *name, int *linenum, EnvStack *stack) {
    getc(stream);  // Consume '='
    Result *result = parse_start(stream, linenum, stack, 2, '\n', ';');
    add_stack_var(stack, name, result->output);
    free(name);
    return result;
}

Result *parse_command(FILE *stream, char *name, int *linenum, EnvStack *stack) {
    char **argv = parse_args(stream, name, linenum, stack);

    push_stack(stack, argv);
    exit_t code = run(name, argv);
    set_last_exit_code(stack, code);
    pop_stack(stack);
    free(name);
    return create_cmd_result("", code, STDOUT_FILENO);
}

char **parse_args(FILE *stream, char *command, int *linenum, EnvStack *stack) {
    long conf_max_args = sysconf(_SC_ARG_MAX);
    size_t max_args = 64;
    if (conf_max_args > 0) max_args = (size_t) conf_max_args;

    char **argv_buf = must_malloc(sizeof *argv_buf * max_args);
    argv_buf[0] = must_strdup(command);
    size_t argc = 1;

    char* arg = "";
    char c;
    bool complete_arg = false;
    while((c = peek_char(stream)) != EOF && c != '\n' && c != ';') {
        switch(c) {
            case '#':
                seek_onto_newline(stream, linenum);
                goto finish;

            case ' ':
                seek_for_spaces(stream);
                complete_arg = false;
                break;

            case '\n':
                fprintf(stderr, "\\n cannot be arg.\n");
                assert(false);

            case '$':
                getc(stream);
                arg = parse_var(stream);
                complete_arg = true;
                break;

            case '"':
                arg = parse_string(stream, linenum, stack);
                complete_arg = true;
                break;

            default:
                c = seek_until_chars(stream, &arg, 3, ' ', '\n', ';');
                complete_arg = true;
        }
        if (argc >= max_args) die_invalid_syntax("Too many args", *linenum);
        if (complete_arg) {
            argv_buf[argc++] = arg;
        }
    }

finish:
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
