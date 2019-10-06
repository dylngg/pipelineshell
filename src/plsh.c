#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

#include "errors.h"
#include "context.h"

void parse_start(FILE *stream, char c, int *linenum);
void parse_action(FILE *stream, int *linenum);
void parse_assignment(FILE *stream, int *linenum);

int main(int argc, char *argv[]) {

    if (argc < 2) {
        fprintf(stderr, "No filename given\n");
        exit(1);
    }
    char *filename = argv[1];

    FILE *stream = fopen(filename, "r");
    if (!stream) {
        perror("File could not be read");
        return 1;
    }

    int linenum = 1;
    char c = getc(stream);
    do {
        printf("%c\n", c);
        parse_start(stream, c, &linenum);
    } while((c = getc(stream)) != EOF);

    fclose(stream);
    return 0;
}

void parse_start(FILE *stream, char c, int *linenum) {
    if (isalpha(c)) {
        parse_action(stream, linenum);
        return;
    }

    switch(c) {
        case '\n':
            (*linenum)++;
        case ';':
            break;

        case '#':
            seek_onto_newline(stream);
            fprintf(stderr, "Seeking util char\n");
            (*linenum)++;
            break;

        case ' ':
            seek_spaces(stream);
            break;

        //case '"':
            // FIXME: What if multiple lines
            //break;

        //case '(':
            // FIXME: What if multiple lines
            //break;

        //case '{':
            // FIXME: What if multiple lines

        //case '{':

        default:
            die_invalid_syntax("Invalid start character", *linenum);
    }
    return;
}

