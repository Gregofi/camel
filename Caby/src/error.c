#include <stdio.h>
#include <stdbool.h>
#include "common.h"
#include "error.h"

#define LINE_LEN_INIT 100

char* readline(FILE *f, size_t* linesize, bool* eof) {
    *eof = false;
    size_t cap = LINE_LEN_INIT;
    char* line = malloc(cap);
    size_t size = 0;
    for (char c = fgetc(f); c != '\n'; c = fgetc(f)) {
        if (c == EOF) {
            *eof = true;
            break;
        }
        line = handle_capacity(line, size, &cap, sizeof(*line));
        line[size++] = c;
    }
    if (linesize != NULL) {
        *linesize = size;
    }
    line[size] = '\0';
    return line;
}

void print_error(const char* filename, struct loc location, const char* format, va_list args) {
    FILE* f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Unable to open source.\n");
        exit(-1);
    }
    size_t line = 0;
    // Find the line where the error begins
    char* line_start;
    size_t linesize;
    for (;;) {
        bool eof;
        line_start = readline(f, &linesize, &eof);
        if (linesize > location.begin) {
            break;
        }
        free(line_start);
        line += 1;
        // + 1 because readline throws away the newline symbol
        location.begin -= linesize + 1;
        location.end -= linesize + 1;
        if (eof) {
            break;
        }
    }

    // TODO: For now, pretend that the error is on single line only
    fprintf(stderr, "%s:%lu:%llu: Fatal: ", filename, line + 1, location.begin + 1);
    vfprintf(stderr, format, args);
    fputc('\n', stderr);

    fprintf(stderr, " | %s\n   ", line_start);
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
