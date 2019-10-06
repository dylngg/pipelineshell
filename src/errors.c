#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "errors.h"

char *strerr(err code) {
    switch(code) {
        case NO_SUCH_VAR:
            return "No such variable";

        case UNKNOWN_CMD:
            return "Unknown command";

        case NO_CONTEXT_END:
            return "Ending context could not be found.";

        case NO_MEM:
            return "Cannot allocate memory";

        default:
            return "";
    }
    return "";
}

void perr(char *msg, err code) {
    fprintf(stderr, "%s: %s", strerr(code), msg);
}

void die_invalid_syntax(char *msg, int linenum) {
    fprintf(stderr, "Invalid Syntax [line %d]: %s\n", linenum, msg);
    exit(1);
}

void die_no_mem() {
    errno = ENOMEM;
    perror("");
    exit(1);
}
