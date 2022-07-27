#include "common.h"

size_t get_cap(size_t curr) {
    return curr < INIT_CAP ? INIT_CAP : curr * 2;
}

void* handle_capacity(void* array, size_t len, size_t* cap) {
    if (len >= *cap) {
        *cap = get_cap(*cap);
        array = realloc(array, *cap);
    }
    return array;
}
