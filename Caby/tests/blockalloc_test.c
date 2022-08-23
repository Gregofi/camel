#include "test.h"
#include "../src/memory/block_alloc.h"

TEST(Allocation) {
    init_heap(1024);
    int* data1 = heap_alloc(64);
    int* data2 = heap_alloc(64);
    ASSERT_W(data1 != NULL && data2 != NULL && data1 != data2);
    for (size_t i = 0; i < 64 / sizeof(*data1); ++i) {
        data1[i] = 1;
        data2[i] = 2;
    }
    for (size_t i = 0; i < 64 / sizeof(*data1); ++i) {
        ASSERT_EQ(data1[i], 1);
        ASSERT_EQ(data2[i], 2);
    }
    done_heap();
    return 0;
}

TEST(ComplicatedAllocations) {
    init_heap(1024);
    int* data1 = heap_alloc(512);
    ASSERT_W(data1 != NULL);
    
    ASSERT_W(heap_alloc(512) == NULL);
    
    int* data2 = heap_alloc(256);
    ASSERT_W(data2 != NULL);
    ASSERT_W(heap_alloc(256) == NULL);
    
    int* data3 = heap_alloc(64);
    ASSERT_W(data3 != NULL);
    ASSERT_W(heap_alloc(184) == NULL);
    
    for (size_t i = 0; i < 512 / sizeof(*data1); ++i) {
        data1[i] = 1;
    }
    for (size_t i = 0; i < 256 / sizeof(*data2); ++i) {
        data2[i] = 2;
    }
    for (size_t i = 0; i < 64 / sizeof(*data3); ++i) {
        data3[i] = 3;
    }

    for (size_t i = 0; i < 512 / sizeof(*data1); ++i) {
        ASSERT_EQ(data1[i], 1);
    }
    for (size_t i = 0; i < 256 / sizeof(*data2); ++i) {
        ASSERT_EQ(data2[i], 2);
    }
    for (size_t i = 0; i < 64 / sizeof(*data3); ++i) {
        ASSERT_EQ(data3[i], 3);
    }
    done_heap();
    return 0;
}

TEST(Freeing) {
    init_heap(1024);
    int* data1 = heap_alloc(400);
    int* data2 = heap_alloc(400);
    ASSERT_W(data1 != NULL && data2 != NULL && heap_alloc(400) == NULL);
    heap_free(data1);
    data1 = heap_alloc(400);
    ASSERT_W(data1 != NULL);
    heap_free(data1);
    heap_free(data2);

    int *data[10];
    for (size_t i = 0; i < 10; ++i) {
        data[i] = heap_alloc(72);
        ASSERT_W(data[i] != NULL);
        data[i][1] = i;
    }

    for (size_t i = 0; i < 10; i+=2) {
        heap_free(data[i]);
    }

    for (size_t i = 1; i < 10; i+=2) {
        ASSERT_W(data[i][1] == i);
    }

    for (size_t i = 0; i < 10; i+=2) {
        data[i] = heap_alloc(72);
        ASSERT_W(data[i] != NULL);
    }
    done_heap();
    return 0;
}





int main() {
    RUN_TEST(Allocation);
    RUN_TEST(ComplicatedAllocations);
    RUN_TEST(Freeing);
    return 0;
}
