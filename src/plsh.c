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
char **extract_args(char *string, char *command, int *linenum, EnvStack *stack);
char *extract_string(char *string, int *linenum, EnvStack *stack);
char *extract_var(char *var, EnvStack *stack);

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
        result = parse_start(stream, linenum, stack, 0);
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
            getc(stream);  // Ignore leading '"'
            if((c = seek_until_chars(stream, &tmp, 2, '\n', '\"')) == '\n' || c == EOF)
                die_invalid_syntax("Expected fully quoted '\"'", *linenum);
            getc(stream);  // Consume ending '"'
            result = create_result(extract_string(tmp, linenum, stack));
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
    char *args_string = NULL;
    seek_until_chars(stream, &args_string, 3, '\n', ';', '#');
    char **argv = extract_args(args_string, name, linenum, stack);
    free(args_string);
    free(name);

    push_stack(stack, argv);
    exit_t code = run(name, argv);
    set_last_exit_code(stack, code);
    pop_stack(stack);
    free(name);
    return create_cmd_result("", code, STDOUT_FILENO);
}

char **extract_args(char *string, char *command, int *linenum, EnvStack *stack) {
    assert(string);
    long conf_max_args = sysconf(_SC_ARG_MAX);
    size_t max_args = 64;
    if (conf_max_args > 0) max_args = (size_t) conf_max_args;

    char **argv_buf = must_malloc(sizeof *argv_buf * max_args);
    argv_buf[0] = must_strdup(command);
    size_t argc = 1;

    bool reached_end = false;
    bool empty = false;
    char *arg = NULL;
    bool complete_arg = false;
    char *string_end = string + strlen(string);
    while(string < string_end) {
        complete_arg = false;
        switch(*string) {
            case '\t':
            case ' ':
                break;

            case '$':
                // TODO: Handle $(...)
                string++;
                arg = strsep(&string, " \t");

                reached_end = (string == NULL);
                if (reached_end) string = string_end;

                empty = (*arg == '\0');
                if (empty)
                    die_invalid_syntax("Expected variable after '$'", *linenum);

                arg = extract_var(arg, stack);
                complete_arg = true;
                break;

            case '"':
                string++;
                arg = strsep(&string, "\"");

                reached_end = (string == NULL);
                if (reached_end) string = string_end;

                empty = (*arg == '\0');
                if (empty)
                    die_invalid_syntax("Expected fully quoted '\"'", *linenum);

                arg = extract_string(arg, linenum, stack);
                complete_arg = true;
                break;

            default:
                arg = strsep(&string, " \t");
                empty = (*arg == '\0');
                assert(!empty);  // Should be handled by ' ' and '\t' cases

                reached_end = (string == NULL);
                if (reached_end) string = string_end;

                arg = must_strdup(arg);
                complete_arg = true;
                break;
        }
        assert(string);
        string++;
        if (argc >= max_args) die_invalid_syntax("Too many args", *linenum);
        if (complete_arg) argv_buf[argc++] = arg;
    }
    argv_buf[argc] = NULL;
    argv_buf = must_realloc(argv_buf, sizeof(*argv_buf) * (argc + 1));
    return argv_buf;
}

char *extract_string(char *string, int *linenum, EnvStack *stack) {
    StrBuilder *build = str_build_create();
    assert(string);
    assert(string[0] != '"' && string[strlen(string) - 1] != '"');

    bool reached_end = false;
    bool empty = false;
    char *tmp = NULL;
    char *string_end = string + strlen(string);
    while(string < string_end) {
        switch(*string) {
            case '$':
                string++;
                tmp = strsep(&string, " \t");

                reached_end = (string == NULL);
                if (reached_end) string = string_end;

                empty = (*tmp == '\0');
                if (empty)
                    str_build_add_c(build, '$');
                else
                    str_build_add_str(build, tmp);
                break;

            case '\\':
                string++;
                switch(*string) {
                    case 'n': *string = '\n'; break;
                    case 't': *string = '\t'; break;
                    case '\\': *string = '\\'; break;
                    case '"': *string = '"'; break;
                    case '*': *string = '*'; break;
                    case '$': *string = '$'; break;
                }
                if (*string != '\0') str_build_add_c(build, *string);
                break;

            default:
                str_build_add_c(build, *string);
                break;
        }
        assert(string);
        string++;
    }
    return str_build_to_str(build);
}

char *extract_var(char *var, EnvStack *stack) {
    assert(var);
    assert(var[0] != '$');
    return strdup(var);
}
