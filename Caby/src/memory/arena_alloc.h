#pragma once
#include "src/common.h"
#include <stdlib.h>

#define GALLOC_INIT_SIZE 4 * 1024 * 1024 // 4MB

struct ArenaAllocator {
    void* mempool;
    size_t size;
    size_t taken;
};

void* arena_push(struct ArenaAllocator* heap, u64 size, u64 align);
struct ArenaAllocator arena_init();
void arena_done(struct ArenaAllocator* heap);
u64 arena_bp(struct ArenaAllocator* heap);
void arena_restore(struct ArenaAllocator* heap, u64 bp);
void* arena_move(struct ArenaAllocator* dest, struct ArenaAllocator* from, u64 begin);
