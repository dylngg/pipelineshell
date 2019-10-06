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

#define RESULT_BUF_SIZE 32

void push_stack(EnvStack *stack, char* argv[]) {
    if (!stack) {
        stack = malloc(sizeof *stack);
        if (!stack) die_no_mem();
        stack->env_stack = NULL;
        stack->nstacks = 0;
        stack->last_exit_code = 0;
    }
    stack->env_stack = realloc(stack->env_stack, sizeof(Env *) * (stack->nstacks + 1));
    if (!stack->env_stack) die_no_mem();

    Env *new = malloc(sizeof *new);
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
    stack->env_stack = realloc(stack->env_stack,
                               sizeof(*(stack->env_stack)) * stack->nstacks);
    if (!stack->env_stack) die_no_mem();
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
    top->names = realloc(top->names, sizeof *(top->names) * top->nvals + 1);
    top->names[top->nvals] = strdup(name);
    top->values = realloc(top->values, sizeof *(top->values) * top->nvals + 1);
    top->values[top->nvals] = strdup(value);
    top->nvals++;
    return;
}

int get_last_exit_code(EnvStack* stack) {
    return stack->last_exit_code;
}

void update_last_exit_code(EnvStack* stack, int exit_code) {
    stack->last_exit_code = exit_code;
}

char seek_onto_chars(FILE *stream, char *result[], int nchars, ...) {
    assert(result);
    char *buf = NULL;
    size_t bufsize = RESULT_BUF_SIZE;
    buf = malloc(sizeof *buf * bufsize);
    if (!buf) die_no_mem();

    va_list ap;
    va_start(ap, nchars);
    char stop_chars[nchars];
    for (int i = 0; i < nchars; i++) stop_chars[i] = va_arg(ap, int);
    va_end(ap);

    char c;
    size_t curr_size = 0;
    while((c = getc(stream)) != EOF) {
        // + 2 for curr char and null terminator
        if (curr_size + 2 >= bufsize) {
            bufsize *= 2;
            // FIXME: Non-GNU will not free new alloc
            buf = realloc(buf, bufsize);
            if (!buf) die_no_mem();
        }
        buf[curr_size] = c;

        for (int j = 0; j < nchars; j++) {
            if (c == stop_chars[j]) goto finish;
        }
        curr_size++;
    }

finish:
    buf[curr_size] = '\0';
    // Free any memory we don't need.
    buf = realloc(buf, curr_size);
    if (!buf) die_no_mem();
    *result = buf;
    return c;
}

void seek_spaces(FILE *stream) {
    char c;
    while((c = getc(stream)) != EOF && isspace(c));
    ungetc(c, stream); // Undo pulling of non space character
}

void seek_onto_newline(FILE *stream) {
    char c;
    while((c = getc(stream)) != EOF && c != '\n');
}

bool contains_before_chars(FILE *stream, char want, int nchars, ...) {
    va_list ap;
    va_start(ap, nchars);
    char stop_chars[nchars];
    for (int i = 0; i < nchars; i++) stop_chars[i] = va_arg(ap, int);
    va_end(ap);

    char c = '\0';
    bool contains = false;
    size_t peeked = 0;
    do {
        if (c == want) {
            contains = true;
            break;
        }
        for (int j = 0; j < nchars; j++) {
            if (c == stop_chars[j]) break;
        }
        peeked += sizeof(c);
    } while((c = getc(stream)) != EOF);

    // Go back to where we were
    fseek(stream, -peeked, SEEK_CUR);
    return contains;
}
