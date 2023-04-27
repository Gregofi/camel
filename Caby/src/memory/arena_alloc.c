#include "arena_alloc.h"
#include "src/common.h"
#include <stdio.h>
#include <stddef.h>
#include <stdalign.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

void* arena_push(struct ArenaAllocator* heap, u64 size, u64 align) {
    size = (size + (align - 1)) & ~(align - 1);
    // TODO: Returning NULL might seem better but we would
    // have to ask every time if we run out of memory.
    // The allocator will use resizing in the future anyway,
    // so this part will go away then.
    if (heap->taken + size > heap->size) {
        fprintf(stderr, "Arena heap: Out of memory (heap size: %lu, to allocate: %lu)\n", heap->size, size);
        exit(-1);
    }

    void* new_block = heap->mempool + heap->taken;
    heap->taken += size;
    return new_block;
}

struct ArenaAllocator arena_init() {
    struct ArenaAllocator heap;
    heap.size = GALLOC_INIT_SIZE;
    heap.taken = 0;
    heap.mempool = malloc(GALLOC_INIT_SIZE);
    return heap;
}

void arena_done(struct ArenaAllocator* heap) {
    free(heap->mempool);
}

u64 arena_bp(struct ArenaAllocator* heap) {
    return heap->taken;
}

void arena_restore(struct ArenaAllocator* heap, u64 bp) {
    heap->taken = bp;
}

 void* arena_move(struct ArenaAllocator* dest, struct ArenaAllocator* from, u64 begin) {
    unsigned char* p = arena_push(dest, from->taken - begin, sizeof(max_align_t));
    for (size_t i = begin; i < from->taken; ++i) {
        p[i - begin] = ((unsigned char*)from->mempool)[i];
    }
    return p;
}
