#include "memory.h"
#include "memory/block_alloc.h"
#include "vm.h"

#include <stdlib.h>
#include <string.h>


void* vmalloc(vm_t* vm, size_t size) {
    if (vm != NULL && mem_taken() > vm->gc.next_gc) {
       vm->gc.next_gc *= GC_HEAP_GROW_FACTOR;
    }
    return heap_alloc(size);
}

/**
 * @param num Number of objects
 * @param size Size of each object
 */
void* vcalloc(vm_t* vm, size_t num, size_t size) {
    void* mem = vmalloc(vm, num * size);
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
