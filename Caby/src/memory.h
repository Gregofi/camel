#pragma once

#include <stdlib.h>
#include <stdbool.h>

void* vmalloc(size_t size);

/**
 * @param num Number of objects
 * @param size Size of each object
 */
void* vcalloc(size_t num, size_t size);

void vfree(void* ptr);

