#ifndef UTILS_H
#define UTILS_H
#include <stddef.h>

typedef struct StrBuilder {
    char *buf;
    size_t bufsize;
    size_t size;
} StrBuilder;

/*
 * Creates a StrBuilder struct that can be used to build a string.
 */
StrBuilder *str_build_create();

/*
 * Adds a character to the StrBuilder.
 */
void str_build_add_c(StrBuilder *build, char c);

/*
 * Adds a string to the StrBuilder.
 */
void str_build_add_str(StrBuilder *build, char *str);

/*
 * Adds a substring to the StrBuilder.
 */
void str_build_add_substr(StrBuilder *build, char *str, int start, int end);

/*
 * Returns a string from the StrBuilder.
 */
char *str_build_to_str(StrBuilder *build);

/*
 * Mallocs and dies if ENOMEM.
 */
void *must_malloc(size_t size);

/*
 * Reallocs and dies if ENOMEM.
 */
void *must_realloc(void *ptr, size_t size);

/*
 * Strdups and dies if ENOMEM.
 */
char *must_strdup(char *string);

/*
 * Copies argv and it's contents.
 */
char **copy_argv(char *argv[], int argc);

/*
 * Prints argv for debugging purposes.
 */
void print_argv(char *argv[]);

#endif // UTILS_H
