#ifdef IMPLEMENT_TESTCASES

#include <common/arena.h>

#if !defined(UNDEF_ALL) && !defined(USE_SBUFF) && !defined(USE_ARENA)
#error "You must define either USE_SBUFF or USE_ARENA to declare the tests, or UNDEF_ALL to undefine all macros"
#endif

#ifndef TEST_NAME

#define TEST_NAMEXX(prefix, test_name) test_##prefix##_##test_name
#define TEST_NAMEX(prefix, test_name) TEST_NAMEXX(prefix, test_name)
#define TEST_NAME(test_name) TEST_NAMEX(TEST_PREFIX, test_name)

#endif

#ifdef USE_SBUFF

#define ALLOC(size) sbuff_alloc(size)
#define CALLOC(count, size) sbuff_calloc(count, size)
#define REALLOC(ptr, size) sbuff_realloc(ptr, size)
#define FREE(ptr) sbuff_free(ptr)
#define RESET() sbuff_reset()
#define SAVE() sbuff_save()
#define REWIND(checkpoint) sbuff_rewind(checkpoint)

#endif 

#ifdef USE_ARENA

#ifndef TEMP_ARENA_NAME 

#error "You must define TEMP_ARENA_NAME to give a name to the temporary arena"

#endif

#define ALLOC(size) arena_alloc(TEMP_ARENA_NAME, size)
#define CALLOC(count, size) arena_calloc(TEMP_ARENA_NAME, count, size)
#define REALLOC(ptr, size) arena_realloc(TEMP_ARENA_NAME, ptr, size)
#define FREE(ptr) arena_free(TEMP_ARENA_NAME, ptr)
#define RESET() arena_reset(TEMP_ARENA_NAME)
#define SAVE() arena_save(TEMP_ARENA_NAME)
#define REWIND(checkpoint) arena_rewind(TEMP_ARENA_NAME, checkpoint)

#endif

#ifdef UNDEF_ALL

#undef TEST_PREFIX
#undef CREATE_ARENA
#undef ALLOC
#undef CALLOC
#undef REALLOC
#undef FREE
#undef RESET
#undef SAVE
#undef REWIND

#endif

#endif // IMPLEMENT_TESTCASES
