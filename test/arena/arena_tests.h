#ifdef IMPLEMENT_TESTCASES

#ifndef TEST_PREFIX
# error "You must define TEST_PREFIX to give a name to the test suite"
#endif

#include "arena_functions.h"

#ifdef USE_ARENA

static arena_t* TEMP_ARENA_NAME = NULL;

#endif // USE_ARENA

#define ALLOCATION_END_POSITION(base, size) \
    (base + sizeof(size_t) + ARENA_ALIGN_SIZE(size))

#define TEST_CASE(testfn) cmocka_unit_test_setup_teardown(testfn, TEST_NAME(verify), TEST_NAME(reset))

static void TEST_NAME(alloc)(void**) {
    size_t before = SAVE();
    void* ptr = ALLOC(20);
    assert_non_null(ptr);
    assert_uint_equal(SAVE(), ALLOCATION_END_POSITION(before, 20));
}

static void TEST_NAME(calloc)(void**) {
    size_t before = SAVE();
    void* ptr = CALLOC(4, 8);
    assert_non_null(ptr);
    assert_uint_equal(SAVE(), ALLOCATION_END_POSITION(before, 4 * 8));
}

static void TEST_NAME(realloc)(void**) {
    size_t before = SAVE();
    void* ptr = REALLOC(NULL, 20);
    assert_non_null(ptr);
    assert_uint_equal(SAVE(), ALLOCATION_END_POSITION(before, 20));
}

static void TEST_NAME(realloc_decrease)(void**) {
    size_t before = SAVE();
    size_t size = 20;
    
    char* ptr = ALLOC(size);
    assert_non_null(ptr);
    assert_uint_equal(SAVE(), ALLOCATION_END_POSITION(before, size));
    for (size_t i = 0; i < size; i++) {
        ptr[i] = i + 1;
    }

    void* new_ptr = REALLOC(ptr, 16);
    assert_non_null(ptr);
    assert_ptr_equal(ptr, new_ptr);
    assert_uint_equal(SAVE(), ALLOCATION_END_POSITION(before, 20));
    assert_memory_equal(ptr, new_ptr, size);

}

static void TEST_NAME(realloc_increase_inplace)(void**) {
    size_t before = SAVE();
    size_t size = 20;

    char* ptr0 = ALLOC(size);
    assert_non_null(ptr0);
    assert_uint_equal(SAVE(), ALLOCATION_END_POSITION(before, size));
    for (size_t i = 0; i < size; i++) {
        ptr0[i] = i + 1;
    }

    void* new_ptr = REALLOC(ptr0, 64);
    assert_non_null(new_ptr);
    assert_ptr_equal(ptr0, new_ptr);
    assert_uint_equal(SAVE(), ALLOCATION_END_POSITION(before, size));
    assert_memory_equal(ptr0, new_ptr, size);
    
}

static void TEST_NAME(realloc_increase)(void**) {
    size_t before0 = SAVE();
    
    size_t size0 = 20;
    char* ptr0 = ALLOC(size0);
    assert_non_null(ptr0);
    assert_uint_equal(SAVE(), ALLOCATION_END_POSITION(before0, size0));
    for(size_t i = 0; i < size0; i++) {
        ptr0[i] = i + 1;
    }
    
    size_t size1 = 12;
    size_t before1 = SAVE();
    void* ptr1 = ALLOC(size1);
    assert_non_null(ptr1);
    assert_uint_equal(SAVE(), ALLOCATION_END_POSITION(before1, size1));

    size_t size2 = 64;
    size_t before2 = SAVE();
    void* new_ptr = REALLOC(ptr0, size2);
    assert_non_null(new_ptr);
    assert_ptr_not_equal(ptr0, new_ptr);
    assert_uint_equal(SAVE(), ALLOCATION_END_POSITION(before2, size2));
    assert_memory_equal(ptr0, new_ptr, size0);

}

static void TEST_NAME(reuse)(void**) {
    size_t start = SAVE();
    size_t size = 40;
    void* ptr = ALLOC(size);
    assert_non_null(ptr);
    assert_uint_equal(SAVE(), ALLOCATION_END_POSITION(start, size));

    size_t assert_post = SAVE();
    ptr = ALLOC(size);

    size_t before = SAVE();

    ptr = ALLOC(size);
    ptr = ALLOC(size);

    REWIND(before);

    assert_int_equal(SAVE(), ALLOCATION_END_POSITION(assert_post, size));
}

static int TEST_NAME(verify)(void**) {
    assert_uint_equal(SAVE(), sizeof(arena_t));
    return 0;
}

static int TEST_NAME(reset)(void**) {
    RESET();
    return 0;
}

#ifndef TEST_SUITE_FUNCTION

#define TEST_SUITE_FUNCTION_XX(prefix) run_##prefix##_##tests
#define TEST_SUITE_FUNCTION_X(prefix) TEST_SUITE_FUNCTION_XX(prefix)
#define TEST_SUITE_FUNCTION() TEST_SUITE_FUNCTION_X(TEST_PREFIX)

#endif

static int TEST_NAME(init_suite)(void** state) {
#ifdef USE_ARENA
    TEMP_ARENA_NAME = arena_create(1024 * 1024); // 1 MB arena
    assert_non_null(TEMP_ARENA_NAME);
#endif // USE_ARENA
    return TEST_NAME(reset)(state);
}

static int TEST_NAME(teardown_suite)(void** state) {
    (void) TEST_NAME(verify)(state);
#ifdef USE_ARENA
    arena_destroy(TEMP_ARENA_NAME);
    TEMP_ARENA_NAME = NULL;
#endif // USE_ARENA
    return 0;
}

int TEST_SUITE_FUNCTION()(void) {
    const struct CMUnitTest sbuff_tests[] = {
        TEST_CASE(TEST_NAME(alloc)),
        TEST_CASE(TEST_NAME(calloc)),
        TEST_CASE(TEST_NAME(realloc)),
        TEST_CASE(TEST_NAME(realloc_decrease)),
        TEST_CASE(TEST_NAME(realloc_increase_inplace)),
        TEST_CASE(TEST_NAME(realloc_increase)),
        TEST_CASE(TEST_NAME(reuse)),
    };

    return cmocka_run_group_tests(sbuff_tests, TEST_NAME(init_suite), TEST_NAME(teardown_suite));
}

#define UNDEF_ALL

#include "arena_functions.h"

#undef UNDEF_ALL


#endif // IMPLEMENT_TESTCASES
