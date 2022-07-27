#pragma once

#include <stdlib.h>

void* vmalloc(size_t size);

void* vrealloc(void* ptr, size_t new_size);

/**
 * @param num Number of objects
 * @param size Size of each object
 */
void* calloc(size_t num, size_t size);

void vfree(void* ptr);