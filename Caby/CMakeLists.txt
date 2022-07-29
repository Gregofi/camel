cmake_minimum_required(VERSION 3.23)

project(caby C)

set(CMAKE_CXX_FLAGS_DEBUG_INIT "-g -Wall -Wextra -pedantic")
set(CMAKE_CXX_FLAGS_RELEASE_INIT "-O2")

add_compile_options(-fsanitize=address)
add_link_options(-fsanitize=address)

add_executable(caby src/main.c src/dissasembler.c src/bytecode.c
                    src/common.c src/object.c src/memory.c src/vm.c
                    src/serializer.c)
