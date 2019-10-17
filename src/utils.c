#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "utils.h"
#include "errors.h"

#define STR_BUF_SIZE 64

static void ensure_str_build_bounds(StrBuilder *build, size_t by) {
    size_t needed_size = build->size + by + 1;  // Plus null terminator
    if (needed_size > build->bufsize) {
        build->bufsize = needed_size * 2;
        build->buf = must_realloc(build->buf, build->bufsize);
    }
}

StrBuilder *str_build_create() {
    StrBuilder *build = must_malloc(sizeof *build);
    build->buf = must_malloc(STR_BUF_SIZE);
    build->bufsize = STR_BUF_SIZE;
    build->size = 0;
    return build;
}

void destroy_str_build(StrBuilder *build) {
    assert(build);
    free(build->buf);
    free(build);
}

void str_build_add_c(StrBuilder *build, char c) {
    assert(build);
    ensure_str_build_bounds(build, 1);
    build->buf[build->size++] = c;
    build->buf[build->size] = '\0';
}

void str_build_add_str(StrBuilder *build, char *str) {
    assert(build);
    assert(str);
    int len = strlen(str);
    ensure_str_build_bounds(build, len);
    // '<=': Include null terminator
    for (int i = 0; i <= len; i++) build->buf[build->size++] = str[i];
}

char *str_build_to_str(StrBuilder *build) {
    build->buf = must_realloc(build->buf, build->size + 1);  // Plus null terminator
    return build->buf;
}

void *must_malloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) die_no_mem();
    return ptr;
}

void *must_realloc(void *ptr, size_t size) {
    // FIXME: Non-GNU will not free new alloc
    ptr = realloc(ptr, size);
    if (!ptr) die_no_mem();
    return ptr;
}

char *must_strdup(char *string) {
    string = strdup(string);
    if (!string) die_no_mem();
    return string;
}

char **copy_argv(char *argv[], int argc) {
    char **new_argv = malloc(sizeof *new_argv * (argc + 1));
    for (int i = 0; i < argc; i++) new_argv[i] = must_strdup(argv[i]);
    new_argv[argc] = NULL;
    return new_argv;
}

void print_argv(char *argv[]) {
    for (int i = 0; argv[i] != NULL; i++) printf("\"%s\" ", argv[i]);
    printf("\n");
}
