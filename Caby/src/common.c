#include "common.h"

size_t get_cap(size_t curr) {
    return curr < INIT_CAP ? INIT_CAP : curr * 2;
}

void* handle_capacity(void* array, size_t cnt, size_t* cap, size_t len) {
    if (cnt >= *cap) {
        *cap = get_cap(*cap);
        array = realloc(array, *cap * len);
        if (array == NULL) {
            fprintf(stderr, "out of memory");
            exit(-1);
        }
    }
    return array;
}

struct ostring make_ostring(char* str, size_t len) {
    return (struct ostring){.s = str, .len = len};
}

struct cstring make_cstring(const char* str) {
    return (struct cstring){.s = str, .len = strlen(str)};
}

bool str_eq(const char* s1, const char* s2, size_t s2_size) {
    if (strlen(s1) != s2_size) {
        return false;
    }

    return !strncmp(s1, s2, s2_size);
}

void write_u8(u8* p, u8 data) {
    *p = data;
}

void write_u16(u8* p, u16 data) {
    write_u8(p, data >> 8);
    write_u8(p + 1, data);
}

void write_u32(u8* p, u32 data) {
    write_u16(p, data >> 16);
    write_u16(p + 2, data);
}
