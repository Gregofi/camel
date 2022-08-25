#pragma once
#include <stdlib.h>

#ifdef __GC_DEBUG__
#define GC_LOG(format, ...) do { fprintf(stderr, format, ##__VA_ARGS__); } while (false)
#else
#define GC_LOG(format, ...)
#endif

typedef struct vm_state vm_t;

struct gc_state {
    size_t wl_count;
    size_t wl_capacity;
    struct object** worklist;
};

void init_gc(struct gc_state* gc);

void free_gc(struct gc_state* gc);

void gc_collect(vm_t* vm);
