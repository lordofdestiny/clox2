#ifndef __CLOX2_COMMON_UTIL_H__
#define __CLOX2_COMMON_UTIL_H__

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define UNIMPLEMENTED() \
    do { \
        fprintf(stderr, "%s:%d: '%s' not implemented\n", __FILE__, __LINE__, __func__); \
        abort(); \
    } while (0)

#define massert(cond, msg) assert((cond) && msg)

#endif // __CLOX2_COMMON_UTIL_H__
