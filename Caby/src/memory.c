#include "memory.h"

void* vmalloc(size_t size) {
    return malloc(size);
}

void* vrealloc(void* ptr, size_t new_size) {
    return realloc(ptr, new_size);
}

/**
 * @param num Number of objects
 * @param size Size of each object
 */
void* vcalloc(size_t num, size_t size) {
    return calloc(num, size);
}

void vfree(void* ptr) {
    free(ptr);
}
