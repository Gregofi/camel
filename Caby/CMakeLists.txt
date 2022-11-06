cmake_minimum_required(VERSION 3.18)

project(caby C)

message("Build type: ${CMAKE_BUILD_TYPE}")

set(CMAKE_C_FLAGS_SANITIZERS "-g -fsanitize=address -fsanitize=undefined -D__DEBUG__ -D__MEM_DEBUG__")
set(CMAKE_C_FLAGS_GC "-g -fsanitize=address -fsanitize=undefined -D__DEBUG__ -D__MEM_DEBUG__ -D__GC_DEBUG__ -D__GC_STRESS__")
set(CMAKE_C_FLAGS_GC_TEST "-g -fsanitize=address -fsanitize=undefined -D__GC_STRESS__")
set(CMAKE_C_FLAGS_DEBUG "-g -Wall -Wextra -pedantic -D__DEBUG__")
set(CMAKE_C_FLAGS_RELEASE "-O2")

add_executable(caby src/main.c src/dissasembler.c src/bytecode.c
                    src/common.c src/object.c src/memory.c src/vm.c
                    src/serializer.c src/hashtable.c src/native.c
                    src/memory/block_alloc.c src/gc.c src/error.c)

target_link_libraries(caby m)

enable_testing()

add_executable(hashmap_test tests/hashmap_test.c src/dissasembler.c src/bytecode.c
                    src/common.c src/object.c src/memory.c src/vm.c
                    src/serializer.c src/hashtable.c src/native.c
                    src/memory/block_alloc.c src/gc.c src/error.c)

target_link_libraries(hashmap_test m)

add_executable(blockalloc_test tests/blockalloc_test.c src/memory/block_alloc.c)

add_test(hashmap_test hashmap_test)
add_test(blockalloc_test blockalloc_test)
