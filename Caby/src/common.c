#include "common.h"

size_t get_cap(size_t curr) {
    return curr < INIT_CAP ? INIT_CAP : curr * 2;
}

void* handle_capacity(void* array, size_t cnt, size_t* cap, size_t len) {
    if (cnt >= *cap) {
        *cap = get_cap(*cap);
        array = realloc(array, *cap * len);
    }
    return array;
}
