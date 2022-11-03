#include <stdio.h>
#include "common.h"
#include "error.h"

#define LINE_LEN_INIT 100

char* readline(FILE *f, size_t* linesize) {
    size_t cap = LINE_LEN_INIT;
    char* line = malloc(cap);
    size_t size = 0;
    for (char c = fgetc(f); c != '\n' && c != EOF; c = fgetc(f)) {
        if (size > cap) {
            cap *= 2;
            line = realloc(line, cap);
        }
        line[size++] = c;
    }
    if (linesize != NULL) {
        *linesize = size;
    }
    line[size] = '\0';
    return line;
}

void print_error(const char* filename, struct loc location, const char* format, ...) {
    FILE* f = fopen(filename, "r");
    size_t line = 0;
    // Find the line where the error begins
    char* line_start;
    size_t linesize;
    for (;;) {
        line_start = readline(f, &linesize);
        if (linesize > location.begin || line_start[0] == '\0') {
            break;
        }
        free(line_start);
        line += 1;
        location.begin -= linesize;
        location.end -= linesize;
    }

    // TODO: For now, pretend that the error is on single line only
    fprintf(stderr, "%s:%lu:%lu: Fatal: ", filename, line, location.begin);

    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    fputc('\n', stderr);
    va_end(args);

    fprintf(stderr, " |%s", line_start);
    for (size_t i = 0; i < linesize; ++ i) {
        if (i < location.begin) {
            fputc(' ', stderr);
        } else if (i > location.end) {
            fputc('\n', stderr);
            break;
        } else if (i == location.begin) {
            fputc('^', stderr);
        } else {
            fputc('~', stderr);
        }
    }
    fputc('\n', stderr);
    fclose(f);
    free(line_start);
}
