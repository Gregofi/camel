#include "compiler.h"
#include "lexer.h"
#include <ctype.h>
#include <stdbool.h>
#include <string.h>


void compile(const char* source) {
    init_lexer(source);
}
