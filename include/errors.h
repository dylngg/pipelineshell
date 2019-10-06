#ifndef ERRORS_H
#define ERRORS_H

typedef enum err {
    NO_SUCH_VAR = 1,
    UNKNOWN_CMD,
    NO_CONTEXT_END,
    NO_MEM
} err;

/*
 * Returns the string description of the given error code.
 */
char *strerr(err code);

/*
 * Prints the error corresponding to the given code, with the given message
 * appended:
 *
 * -> "No such variable: msg"
 */
void perr(char *msg, err code);

/*
 * Exits the program with a formatted error message.
 */
void die_invalid_syntax(char *msg, int linenum);

/*
 * Exits the program with a message about no memory.
 */
void die_no_mem();

#endif // ERRORS_H
