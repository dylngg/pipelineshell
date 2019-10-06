#ifndef CONTEXT_H
#define CONTEXT_H
#include <stdio.h>
#include <stdbool.h>

typedef struct Env {
    char **argv;
    char **names;
    char **values;
    int nvals;
} Env;

typedef struct EnvStack {
    int last_exit_code;
    Env **env_stack;
    int nstacks;
} EnvStack;

/*
 * Pushes a new environment to the stack.
 */
void push_stack(EnvStack *stack, char *argv[]);

/*
 * Pops the top environment.
 */
void pop_stack(EnvStack *stack);

/*
 * Returns the current environment.
 */
Env * get_env(EnvStack *stack);

/*
 * Returns a variable from the stack.
 */
char * get_stack_var(EnvStack *stack, char *name);

/*
 * Adds a variable to the stack.
 */
void add_stack_var(EnvStack *stack, char *name, char *value);

/*
 * Returns the last exit code.
 */
int get_last_exit_code(EnvStack *stack);

/*
 * Updates the last exit code.
 */
void update_last_exit_code(EnvStack *stack, int exit_code);

/*
 * Seeks the stream util a given character is found (the given char is
 * consumed), or EOF is found. Returns the last character found and puts the
 * seeked string (without the given char) into the allocated result buffer.
 */
char seek_onto_chars(FILE *stream, char *result[], int nchars, ...);

/*
 * Seeks the stream up to a non-whitespace character (consumes the string, but
 * not the non-whitespace character).
 */
void seek_spaces(FILE *stream);

/*
 * Seeks the stream until a newline character (consumes the newline).
 */
void seek_onto_newline(FILE *stream);

/*
 * Peeks at the stream until either the specified character is found or the
 * additional chars are reached. Returns whether the specififed char is found.
 */
bool contains_before_chars(FILE *stream, char c, int n, ...);

#endif // CONTEXT_H
