cmake_minimum_required(VERSION 3.18)

project(caby C)

message("Build type: ${CMAKE_BUILD_TYPE}")

set(CMAKE_C_FLAGS_SANITIZERS "-g -fsanitize=address -fsanitize=undefined")
set(CMAKE_C_FLAGS_DEBUG "-g -Wall -Wextra -pedantic")
set(CMAKE_C_FLAGS_RELEASE "-O2")

add_executable(caby src/main.c src/dissasembler.c src/bytecode.c
                    src/common.c src/object.c src/memory.c src/vm.c
                    src/serializer.c src/hashtable.c)

enable_testing()

add_executable(hashmap_test tests/hashmap_test.c src/hashtable.c src/object.c
                            src/memory.c src/common.c)

add_test(hashmap_test hashmap_test)
