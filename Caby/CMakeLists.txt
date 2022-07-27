cmake_minimum_required(VERSION 3.23)

project(caby C)

set(CMAKE_CXX_FLAGS_DEBUG_INIT "-Wall -Wextra -pedantic -g -fsanitize=address -fsanitize=undefined")
set(CMAKE_CXX_FLAGS_RELEASE_INIT "-O2")

add_executable(caby src/main.c src/dissasembler.c src/bytecode.c src/common.c)
