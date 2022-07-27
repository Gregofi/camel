#pragma once

#include <stdlib.h>

inline void* vmalloc(size_t size) {
    return malloc(size);
}

inline void* vrealloc(void* ptr, size_t new_size) {
    return realloc(ptr, new_size);
}

/**
 * @param num Number of objects
 * @param size Size of each object
 */
inline void* calloc(size_t num, size_t size) {
    return calloc(num, size);
}

inline void vfree(void* ptr) {
    free(ptr);
}
