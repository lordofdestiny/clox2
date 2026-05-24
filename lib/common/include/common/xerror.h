#ifndef __COMMON_XERROR_H__
#define __COMMON_XERROR_H__

#include <errno.h>

#define REQUIRE_LITERAL(x) \
    static_assert(__builtin_constant_p(x), "Argument must be a string literal")

#define XERROR_LIBC_NAME "__xerror_libc__"

#define XERROR(...) _XERROR_CHOSE(__VA_ARGS__)(__VA_ARGS__)

#define XERROR_LIBC(context) _XERROR_NEW_IMPL(NULL, XERROR_LIBC_NAME, errno, context)

#define _XERROR_CHOSE(...) _XERROR_CHOSE_IMPL(__VA_ARGS__, _XERROR_NESTED, _XERROR_NEW, _XERROR_NOT_SUPPORTED, _XERROR_NEW_LIBC_AUTO_CODE)

#define _XERROR_CHOSE_IMPL(_1, _2, _3, _4, NAME, ...) NAME

#define _XERROR_NEW_IMPL(cause, lib, code, context) xerror_create(cause, lib, code, context, __FILE__, __LINE__, __func__); REQUIRE_LITERAL(lib); REQUIRE_LITERAL(context)

#define _XERROR_NESTED(cause, lib, code, context) _XERROR_NEW_IMPL(cause, lib, code, context);

#define _XERROR_NEW(lib, code, context) _XERROR_NEW_IMPL(NULL, lib, code, context);

#define _XERROR_NEW_LIBC(code, context) static_assert(false, "LIBC error codes are not supported");

typedef struct xerror xerror;

xerror* xerror_create(xerror* cause, const char* lib_name, int code, const char* context, const char* file, int line, const char* function);

void xerror_free(xerror* err);

void xperror(xerror* err);

#endif // __COMMON_XERROR_H__
