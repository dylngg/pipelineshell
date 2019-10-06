#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "context.h"
#include "errors.h"

#define RESULT_BUF_SIZE 1024

char seek_onto_chars(FILE *stream, char *result[], int nchars, ...) {
    char *buf = NULL;
    size_t bufsize = RESULT_BUF_SIZE;
    if (result) {
        buf = malloc(sizeof *buf * bufsize);
        if (!buf) die_no_mem();
    }

    va_list ap;
    va_start(ap, nchars);
    char stop_chars[nchars];
    for (int i = 0; i < nchars; i++) stop_chars[i] = va_arg(ap, int);
    va_end(ap);

    char c;
    size_t curr_size = 0;
    do {
        // Resize buffer
        if (result) {
            // + 2 for curr char and null terminator
            if (curr_size + 2 >= bufsize) {
                bufsize *= 2;
                // FIXME: Non-GNU will not free new alloc
                buf = realloc(buf, bufsize);
                if (!buf) die_no_mem();
            }
            buf[curr_size] = c;
        }

        for (int j = 0; j < nchars; j++) {
            if (c == stop_chars[j]) break;
        }
        curr_size++;

    } while((c = getc(stream)) != EOF);

    if (result) {
        buf[curr_size] = '\0';

        // Free any memory we don't need.
        buf = realloc(buf, curr_size);
        if (!buf) die_no_mem();
        *result = buf;
    }
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

char seek_onto_action_operator_pair(FILE *stream, char** action) {
    char last = seek_onto_chars(stream, action, 2, ' ', '\t', '\n', '=', '|');
    switch(last) {
        case '|':
        case '=':
            return last;

        case '\n':
        case ' ':
        case '\t':
            ungetc(last, stream);
            return 0;  // No action operator pair
        case EOF:
            return 0;
    }
    return 0;
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
