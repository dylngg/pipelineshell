#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
#include "errors.h"

#define STR_BUF_SIZE 64

static void extend_bounds(StrBuilder* build, size_t by) {
    size_t needed_size = build->size + by + 1;  // Plus null terminator
    if (needed_size > build->bufsize) {
        build->bufsize = needed_size * 2;
        // FIXME: Non-GNU will not free new alloc
        build->buf = must_realloc(build->buf, build->bufsize);
    }
}

StrBuilder *str_build_create() {
    StrBuilder *build = must_malloc(sizeof *build);
    build->buf = must_malloc(sizeof *build->buf * STR_BUF_SIZE);
    build->bufsize = STR_BUF_SIZE;
    build->size = 0;
    return build;
}

void str_build_add_c(StrBuilder *build, char c) {
    assert(build);
    extend_bounds(build, 1);
    build->buf[build->size++] = c;
}

void str_build_add_str(StrBuilder *build, char *str) {
    str_build_add_substr(build, str, 0, strlen(str));
}

void str_build_add_substr(StrBuilder *build, char *str, int start, int end) {
    assert(build);
    assert(end >= start);
    size_t len = end - start;
    extend_bounds(build, len);
    for (int i = start; i < end; i++) build->buf[build->size++] = str[i];
}

char *str_build_to_str(StrBuilder* build) {
    // FIXME: Non-GNU will not free new alloc
    build->buf = must_realloc(build->buf, build->size + 1);  // Plus null terminator
    build->buf[build->size] = '\0';
    return build->buf;
}

void *must_malloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) die_no_mem();
    return ptr;
}

void *must_realloc(void* ptr, size_t size) {
    ptr = realloc(ptr, size);
    if (!ptr) die_no_mem();
    return ptr;
}
