#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

#define INIT_CAP 8

size_t get_cap(size_t curr);

// Reads bytes from pointer in little endian
#define READ_2BYTES_LE(value) ( *((u8*)(value)) | *((u8*)(value) + 1) << 8)

#define READ_4BYTES_LE(value) ( *(u8*)(value) | ((*(u8*)(value + 1)) << 8) \
            | ((*(u8*)(value + 2)) << 16) | ((*(u8*)(value + 3)) << 24))

#define READ_BYTE_LE(value) (*((u8*)((value))))

#define READ_4BYTES_BE(value) (((value)[0] << 24) |  ((value)[1] << 16) \
            | ((value)[2] << 8) | ((value)[3]))

#define READ_2BYTES_BE(value) (((value)[0] << 8) | ((value)[1]))

/// Reallocates the array if the capacity is exceeded
/// New array is returned if reallocation occured, else
/// old one is returned. Capacity is updated accordingly.
/// Uses system realloc (ie. memory is not GCed).
///
/// @param cnt - number of objects
/// @param len - size of one object.
void* handle_capacity(void* array, size_t cnt, size_t* cap, size_t len);

#define NOT_IMPLEMENTED() do { fprintf(stderr,                     \
                "Inner compiler error: Not implemented: %s:%d.\n", \
                __FILE__, __LINE__); exit(-1);} while(0)

#define UNREACHABLE() do { fprintf(stderr,                         \
                "Inner compiler error: Unreachable: %s:%d.\n",     \
                __FILE__, __LINE__); exit(-1);} while(0)

// Borrowed from linux kernel
#if __has_attribute(__fallthrough__)
# define fallthrough                    __attribute__((__fallthrough__))
#else
# define fallthrough                    do {} while (0)  /* fallthrough */
#endif
