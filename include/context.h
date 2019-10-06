#ifndef CONTEXT_H
#define CONTEXT_H
#include <stdbool.h>

typedef enum context {
    START,
    VAR,
    LAMBDA,
    LINE_LAMBDA,
    CAPTURE,
    PIPE
} context;

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
 * Seeks the stream for a ACTION OPERATOR pair, where a operator is one of the
 * following: '=', '|'. The operator is returned and the action is stored in
 * **action. The search ends at a newline. If a action operator is not found,
 * a 0 is returned.
 */
char seek_onto_action_operator_pair(FILE *stream, char **action);

/*
 * Peeks at the stream until either the specified character is found or the
 * additional chars are reached. Returns whether the specififed char is found.
 */
bool contains_before_chars(FILE *stream, char c, int n, ...);

#endif // CONTEXT_H
