#pragma once

#include <stdio.h>

#define ASSERT_W(expr)                                                                                      \
do {                                                                                                        \
if(!(expr))                                                                                                 \
{                                                                                                           \
printf("%s:%d - assertion failed in %s\n", __FILE__, __LINE__, __PRETTY_FUNCTION__);                        \
return EXIT_FAILURE;                                                                                        \
}                                                                                                           \
} while (0)

#define ASSERT_EQ(a, b)                                                                                     \
do {                                                                                                        \
if((a) != (b))                                                                                              \
{                                                                                                           \
printf("%d:%d - assertion failed in %s\n", __FILE__, __LINE__, __PRETTY_FUNCTION__);                        \
printf("a=%d, b=%d\n",a,b);                                                                                 \
return EXIT_FAILURE;                                                                                        \
} else { void(0); }                                                                                         \
} while (0)

#define GREEN_COLOR_TERMINAL "\033[0;32m"
#define RED_COLOR_TERMINAL "\033[0;31m"
#define CLEAR_COLOR_TERMINAL "\033[0;0m"


#define TEST(name) int name()

#define RUN_TEST(test)                                                                                      \
do {                                                                                                        \
if((test)() != 0) {                                                                                         \
    printf("Test '%s'" RED_COLOR_TERMINAL " failed.\n" CLEAR_COLOR_TERMINAL, #test);                        \
    return 1;                                                                                               \
} else {                                                                                                    \
    printf("Test '%s'" GREEN_COLOR_TERMINAL " completed.\n" CLEAR_COLOR_TERMINAL, #test);                   \
}}                                                                                                          \
while (0)
