#ifndef EXEC_H
#define EXEC_H

#include "context.h"

typedef struct Result {
    char *output;
    exit_t code;
    int out_fd;
} Result;

/*
 * Creates a result from the given output (duplicates it), exit code and out
 * file descriptor.
 */
Result *create_cmd_result(char *output, exit_t code, int out_fd);

/*
 * Creates a result from the given output, exit code of zero and the stdout
 * file descriptor.
 */
Result *create_result(char *output);

/*
 * Creates a empty result with a empty string, exit code of zero and the
 * stdout file desciptor.
 */
Result *create_empty_result();

/*
 * Destroys the result by deallocating the output and closing the out file
 * descriptor.
 */
void destroy_result(Result *result);

/*
 * Runs a pipeline of commands found by popping the stack the given number of
 * times.
 */
Result *pipeline_cmds(EnvStack *stack, int ncmds);

#endif  // EXEC_H
