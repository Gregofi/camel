#pragma once
#include <stdlib.h>

#ifdef __GC_DEBUG__
#define GC_LOG(format, ...) do { fprintf(stderr, format, ##__VA_ARGS__); } while (false)
#else
#define GC_LOG(format, ...)
#endif

#define GC_THRESHOLD_START 1024 * 1024 // 1kb

typedef struct vm_state vm_t;

struct gc_state {
    size_t wl_count;
    size_t wl_capacity;
    struct object** worklist;

    size_t next_gc;
};

void init_gc(struct gc_state* gc);

void free_gc(struct gc_state* gc);

void gc_collect(vm_t* vm);
