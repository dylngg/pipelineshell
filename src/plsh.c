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

Result *parse_scope(FILE *stream, char *argv[], int *linenum, EnvStack *stack, char *bounds);
Result *parse_start(FILE *stream, int *linenum, EnvStack *stack, char *bounds);
Result *parse_action(FILE *stream, int *linenum, EnvStack *stack);
Result *parse_assignment(FILE *stream, char *name, int *linenum, EnvStack *stack);
Result *parse_command(FILE *stream, char *name, int *linenum, EnvStack *stack);
int prepare_commands(FILE *stream, char *first_cmd, int *linenum, EnvStack *stack);
char **extract_args(char *string, char *command, int *linenum, EnvStack *stack);
char *extract_string(char *string, EnvStack *stack);
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
    Result *result = parse_scope(stream, argv_copy, &linenum, &stack, NULL);
    exit_t code = result->code;
    destroy_result(result);

    fclose(stream);
    return code;
}

Result *parse_scope(FILE *stream, char *argv[], int *linenum, EnvStack *stack, char *bounds) {
    if (!bounds) bounds = "";
    if (argv) push_stack(stack, argv);
    else push_stack_from_prev(stack);

    Result *result = NULL;
    char c;
    while((c = peek_char(stream)) != EOF) {
        if (result) free(result);  // Free previous result
        for (int i = 0; bounds[i] != '\0'; i++) if (c == bounds[i]) goto finish;
        result = parse_start(stream, linenum, stack, NULL);
    }

finish:
    pop_stack(stack);
    return result ? result : create_empty_result();
}

Result *parse_start(FILE *stream, int *linenum, EnvStack *stack, char *bounds) {
    if (!bounds) bounds = "";
    Result *result = NULL;
    char *tmp = "";
    char *tmp2 = "";
    char c;

top:
    c = peek_char(stream);
    for (int i = 0; bounds[i] != '\0'; i++) if (c == bounds[i]) goto finish;
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
            if((c = seek_until_chars(stream, &tmp, "\n\"")) == '\n' || c == EOF)
                die_invalid_syntax("Expected fully quoted '\"'", *linenum);

            getc(stream);  // Consume ending '"'
            tmp2 = extract_string(tmp, stack);
            result = create_result(tmp2);
            free(tmp);
            free(tmp2);
            break;

        case '$':
            getc(stream);  // Ignore leading '$'
            c = seek_until_chars(stream, &tmp, "\n \t;");
            if (strlen(tmp) < 1)
                die_invalid_syntax("Expected variable after '$'", *linenum);

            tmp2 = extract_var(tmp, stack);
            result = create_result(tmp2);
            free(tmp);
            free(tmp2);
            break;

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
            seek_until_chars(stream, &tmp, "\n \t;");
            result = create_result(tmp);
            free(tmp);
            break;
    }
finish:
    return result;
}

Result *parse_action(FILE *stream, int *linenum, EnvStack *stack) {
    char *name = NULL;

    char c = seek_until_chars(stream, &name, "\n \t;=|");
    if (c == ' ' || c == '\t') c = seek_for_spaces(stream);

    if (c == '=')
        // Is a assignment
        return parse_assignment(stream, name, linenum, stack);

    // Is a command
    return parse_command(stream, name, linenum, stack);
}

Result *parse_assignment(FILE *stream, char *name, int *linenum, EnvStack *stack) {
    getc(stream);  // Consume '='
    Result *result = parse_start(stream, linenum, stack, "\n;");
    add_stack_var(stack, name, result->output);
    free(name);
    return result;
}

Result *parse_command(FILE *stream, char *name, int *linenum, EnvStack *stack) {
    int ncmds = prepare_commands(stream, name, linenum, stack);
    Result* result = pipeline_cmds(stack, ncmds);
    set_last_exit_code(stack, result->code);
    return result;
}

int prepare_commands(FILE *stream, char *first_cmd, int *linenum, EnvStack *stack) {
    // FIXME: Passing the command name in is kinda ugly, we should parse the
    //        command name in this function.
    char *args_string = NULL;
    char *next_cmd = NULL;
    int ncmds = 1;

    char c = seek_until_chars(stream, &args_string, "\n;#|");
    char **argv = extract_args(args_string, first_cmd, linenum, stack);
    free(args_string);
    free(first_cmd);

    if (c == '|') {
        getc(stream);  // Consume pipe
        seek_for_spaces(stream);  // Consume extra space between pipe and cmd

        c = seek_until_chars(stream, &next_cmd, "\n \t;");
        if (c == ' ' || c == '\t')
            seek_for_spaces(stream);
        else if (strlen(next_cmd) < 1)
            die_invalid_syntax("Expected command after '|'", *linenum);

        ncmds += prepare_commands(stream, next_cmd, linenum, stack);
    }
    push_stack(stack, argv);
    return ncmds;
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
                string++;
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
                string++;
                break;

            case '"':
                string++;
                arg = strsep(&string, "\"");

                reached_end = (string == NULL);
                if (reached_end) string = string_end;

                empty = (*arg == '\0');
                if (empty)
                    die_invalid_syntax("Expected fully quoted '\"'", *linenum);

                arg = extract_string(arg, stack);
                complete_arg = true;
                string++;
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
        if (argc >= max_args) die_invalid_syntax("Too many args", *linenum);
        if (complete_arg) argv_buf[argc++] = arg;
    }
    argv_buf[argc] = NULL;
    argv_buf = must_realloc(argv_buf, sizeof(*argv_buf) * (argc + 1));
    return argv_buf;
}

char *extract_string(char *string, EnvStack *stack) {
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
                    str_build_add_str(build, extract_var(tmp, stack));
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
    char *stack_var = get_stack_var(stack, var);
    if (!stack_var) return strdup("");
    return strdup(stack_var);
}
