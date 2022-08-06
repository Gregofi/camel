#pragma once

#include <stdio.h>

#define ASSERT_W(expr)                                                                                      \
if(!(expr))                                                                                                 \
{                                                                                                           \
printf("%s:%d - assertion failed in %s\n", __FILE__, __LINE__, __PRETTY_FUNCTION__);                        \
return EXIT_FAILURE;                                                                                        \
}

#define ASSERT_EQ(a, b)                                                                                     \
if((a) != (b))                                                                                              \
{                                                                                                           \
printf("%d:%d - assertion failed in %s\n", __FILE__, __LINE__, __PRETTY_FUNCTION__);                        \
printf("a=%d, b=%d\n",a,b);                                                                                 \
return EXIT_FAILURE;                                                                                        \
} else void(0)

#define GREEN_COLOR_TERMINAL "\033[0;32m"
#define RED_COLOR_TERMINAL "\033[0;31m"
#define CLEAR_COLOR_TERMINAL "\033[0;0m"


#define TEST(name) int name()

#define RUN_TEST(test)                                                                                          \
if((test)() != 0)                                                                                           \
    printf("Test '%s'" RED_COLOR_TERMINAL " failed.\n" CLEAR_COLOR_TERMINAL, #test);                        \
else {                                                                                                      \
    printf("Test '%s'" GREEN_COLOR_TERMINAL " completed.\n" CLEAR_COLOR_TERMINAL, #test);                   \
}
