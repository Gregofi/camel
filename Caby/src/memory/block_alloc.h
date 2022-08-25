#pragma once
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#define MIN_SPLIT 32
#define MINIMUM_HEAP_SIZE 512lu

struct heap_header {
    size_t len;
    struct heap_header* next;
    bool taken;
};

// #define SET_TAKEN(header) do { (header)->next &= 1; } while (false)
// #define SET_FREE(header) do { (header)->next &= 0; } while (false)
// #define IS_TAKEN(header) ((uintptr_t)(header)->next & 1ULL)
// #define GET_NEXT(header) ((struct heap_header*)(((uintptr_t)(header)->next) & ~(1ULL)))
// #define SET_NEXT(header, ptr) do { (header)->next = (struct heap_header*)(((uintptr_t)(header)->next & 1ULL) & (uintptr_t)(ptr)); } while (false)


void init_heap(size_t size);

void done_heap();

void* heap_alloc(size_t size);

void heap_free(void* ptr);

size_t heap_total();

size_t heap_taken();
