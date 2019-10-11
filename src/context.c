#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "context.h"
#include "errors.h"
#include "utils.h"

#define RESULT_BUF_SIZE 32

void push_stack(EnvStack *stack, char* argv[]) {
    if (!stack) {
        stack = must_malloc(sizeof *stack);
        stack->env_stack = NULL;
        stack->nstacks = 0;
        stack->last_code = 0;
    }
    stack->env_stack = must_realloc(stack->env_stack, sizeof(Env *) * (stack->nstacks + 1));

    Env *new = must_malloc(sizeof *new);
    new->argv = argv;
    new->names = NULL;
    new->values = NULL;
    new->nvals = 0;
    stack->env_stack[stack->nstacks] = new;
    stack->nstacks++;
}

void pop_stack(EnvStack *stack) {
    assert(stack);
    assert(stack->nstacks > 0);

    Env* toremove = stack->env_stack[stack->nstacks - 1];
    free(toremove->argv);
    for (int i = 0; i < toremove->nvals; i++) {
        free(toremove->names[i]);
        free(toremove->values[i]);
    }
    free(toremove->names);
    free(toremove->values);
    free(toremove);
    stack->nstacks--;
    stack->env_stack = must_realloc(stack->env_stack,
                               sizeof(*(stack->env_stack)) * stack->nstacks);
}

Env * get_env(EnvStack *stack) {
    assert(stack);
    assert(stack->nstacks > 0);
    return stack->env_stack[stack->nstacks - 1];
}

char *get_stack_var(EnvStack *stack, char *name) {
    assert(stack);
    for (int i = stack->nstacks - 1; i >= 0; i--) {
        Env *curr = stack->env_stack[i];
        for (int j = 0; j < curr->nvals; j++)
            if (strcmp(curr->names[j], name) == 0) return curr->values[j];
    }
    return NULL;
}

void add_stack_var(EnvStack *stack, char* name, char* value) {
    assert(stack);
    assert(stack->nstacks > 0);
    int top_index = stack->nstacks - 1;

    // Search for existing stack variables
    Env *curr;
    for (int i = top_index; i >= 0; i--) {
        curr = stack->env_stack[i];
        for (int j = 0; j < curr->nvals; j++) {
            if (strcmp(curr->names[j], name) == 0) {
                curr->values[j] = value;
                return;
            }
        }
    }

    // If none, add a new variable
    Env *top = stack->env_stack[top_index];
    top->names = must_realloc(top->names, sizeof *(top->names) * top->nvals + 1);
    top->names[top->nvals] = strdup(name);
    top->values = must_realloc(top->values, sizeof *(top->values) * top->nvals + 1);
    top->values[top->nvals] = strdup(value);
    top->nvals++;
    return;
}

exit_t get_last_exit_code(EnvStack* stack) {
    return stack->last_code;
}

void set_last_exit_code(EnvStack* stack, exit_t code) {
    stack->last_code = code;
}

char peek_char(FILE *stream) {
    char c = getc(stream);
    return ungetc(c, stream);
}

char seek_until_chars(FILE *stream, char *result[], int nchars, ...) {
    assert(result);
    StrBuilder* build = str_build_create();

    va_list ap;
    va_start(ap, nchars);
    char stop_chars[nchars];
    for (int i = 0; i < nchars; i++) stop_chars[i] = va_arg(ap, int);
    va_end(ap);

    char c;
    while((c = peek_char(stream)) != EOF) {
        for (int i = 0; i < nchars; i++)
            if (c == stop_chars[i]) goto finish;
        str_build_add_c(build, c);
        getc(stream);
    }

finish:
    *result = str_build_to_str(build);
    free(build);
    return c;  // Restore the stop char
}

char seek_for_spaces(FILE *stream) {
    char c;
    while((c = getc(stream)) != EOF && c != '\n') continue;
    return ungetc(c, stream); // Undo pulling of newline
}

char seek_for_whitespace(FILE *stream, int *linenum) {
    char c;
    while((c = getc(stream)) != EOF && isspace(c)) if (c == '\n') *linenum = *linenum + 1;
    return ungetc(c, stream); // Undo pulling of non space character
}

void seek_onto_newline(FILE *stream, int *linenum) {
    char c;
    while((c = getc(stream)) != EOF && c != '\n') if (c == '\n') *linenum = *linenum + 1;
}
