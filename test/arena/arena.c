#define UNIT_TESTING 1
#include <cmocka.h>

#define IMPLEMENT_TESTCASES

#define USE_SBUFF
#define TEST_PREFIX sbuff

#include "arena_tests.h"

#undef TEST_PREFIX
#undef USE_SBUFF

#define USE_ARENA
#define TEST_PREFIX arena
#define TEMP_ARENA_NAME temp_arena

#include "arena_tests.h"

#undef TEMP_ARENA_NAME
#undef TEST_PREFIX
#undef USE_ARENA

#undef IMPLEMENT_TESTCASES

int main(void) {
    int failed = run_sbuff_tests();
    failed += run_arena_tests();
    return failed;
}
