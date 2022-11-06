#pragma once
#include <stdio.h>
#include <stdlib.h>
#include "bytecode.h"

/**
 * Writes error message and displays the line(s) in source file where the error happened if source file is provided.
 */
void print_error(const char* filename, struct loc location, const char* format, va_list args);
