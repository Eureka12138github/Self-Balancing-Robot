// my_assert.h
#ifndef MY_ASSERT_H
#define MY_ASSERT_H

#include <stdint.h>
//#include "Serial_V2.h"
#include "delay.h"

#ifdef NDEBUG
    #define MY_ASSERT(expr) ((void)0)
#else
    #define MY_ASSERT(expr) do { \
        if (!(expr)) { \
            my_assert_handler(#expr, __FILE__, __LINE__); \
        } \
    } while(0)
#endif

void my_assert_handler(const char* expr, const char* file, int line);


#endif
