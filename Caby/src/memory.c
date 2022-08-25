#include "memory.h"
#include "memory/block_alloc.h"

#include <stdlib.h>
#include <string.h>


void* vmalloc(size_t size) {
    return heap_alloc(size);
}

/**
 * @param num Number of objects
 * @param size Size of each object
 */
void* vcalloc(size_t num, size_t size) {
    void* mem = vmalloc(num * size);
    memset(mem, 0, num * size);
    return mem;
}

void vfree(void* ptr) {
    heap_free(ptr);
}

size_t mem_taken() {
    return heap_taken();
}

size_t mem_total() {
    return heap_total();
}
