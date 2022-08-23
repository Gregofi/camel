#include "block_alloc.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

/// FIXME: Not thread-safe
void* mempool;
size_t mempool_taken;
size_t mempool_total;

static struct heap_header* init_header(void* at) {
    struct heap_header* h = at;
    h->len = 0;
    h->next = NULL;
    h->taken = false;
    return h;
}

void init_heap(size_t size) {
    if (size < MINIMUM_HEAP_SIZE) {
        fprintf(stderr, "Minimum heap size is %lu", MINIMUM_HEAP_SIZE);
    }
    mempool = malloc(size);
    if (mempool == NULL) {
        fprintf(stderr, "Failed to allocate %lu bytes from OS\n", size);
        exit(-2);
    }

    struct heap_header* header = init_header(mempool);
    header->len = size - sizeof(*header);  
}

void done_heap() {
    free(mempool);
}

void* heap_alloc(size_t size) {
    if (size < MIN_SPLIT) {
        size = MIN_SPLIT;
    }

    struct heap_header* it = mempool;
    while (it->taken || it->len < size) {
        if (it->next != NULL) {
            it = it->next;
        } else {
            break;
        }
    }

    if (it->taken || it->len < size) {
        return NULL;
    }

    it->taken = true;

    // Split this block into another block if the allocation is small enough,
    // if not then keep the original size.
    // Be extra careful here, because all types are unsigned.
    if (it->len - size >= MIN_SPLIT + sizeof(struct heap_header)) {
        struct heap_header* h = init_header((uint8_t*)it + sizeof(*it) + size);
        h->len = it->len - sizeof(*h) - size;
        it->len = size;
        h->next = it->next;
        it->next = h;
    }

    mempool_taken += it->len;
    return (uint8_t*)it + sizeof(*it);
}

void heap_free(void* ptr) {
    struct heap_header* it = (struct heap_header*)((uint8_t*)ptr - sizeof(*it));
    mempool_taken -= it->len;
    it->taken = false;
    struct heap_header* next = it->next;
    // Merge blocks that are next to this one and also free
    while (next != NULL && !next->taken) {
        it->len += sizeof(*next) + next->len;
        it->next = next->next;
        next = next->next;
    }
}
