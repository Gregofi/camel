#pragma once
#include "vm.h"

#ifdef __GC_DEBUG__
#define GC_LOG(format, ...) do { fprintf(stderr, format, ##__VA_ARGS__); } while (false)
#else
#define GC_LOG(format, ...)
#endif

void gc_collect(vm_t* vm);
