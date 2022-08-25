#pragma once

#include <stdlib.h>
#include <stdbool.h>

typedef struct vm_state vm_t;

void* vmalloc(vm_t* vm, size_t size);

/**
 * @param num Number of objects
 * @param size Size of each object
 */
void* vcalloc(vm_t* vm, size_t num, size_t size);

void vfree(void* ptr);

size_t mem_taken();

size_t mem_total();
