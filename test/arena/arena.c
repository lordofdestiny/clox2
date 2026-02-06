#define UNIT_TESTING 1
#include <cmocka.h>

#include "arena.h"

#define ALLOCATION_END_POSITION(base, size) \
    (base + sizeof(size_t) + ARENA_ALIGN_SIZE(size))

static void test_sbuff_alloc(void **) {
    size_t before = sbuff_save();
    void* ptr = sbuff_alloc(20);
    assert_non_null(ptr);
    assert_uint_equal(sbuff_save(), ALLOCATION_END_POSITION(before, 20));
}

static void test_sbuff_calloc(void**) {
    size_t before = sbuff_save();
    void* ptr = sbuff_calloc(4, 8);
    assert_non_null(ptr);
    assert_uint_equal(sbuff_save(), ALLOCATION_END_POSITION(before, 4 * 8));
}

static void test_sbuff_realloc(void**) {
    size_t before = sbuff_save();
    void* ptr = sbuff_realloc(NULL, 20);
    assert_non_null(ptr);
    assert_uint_equal(sbuff_save(), ALLOCATION_END_POSITION(before, 20));
}

static void test_sbuff_realloc_decrease(void**) {
    size_t before = sbuff_save();
    size_t size = 20;
    
    char* ptr = sbuff_alloc(size);
    assert_non_null(ptr);
    assert_uint_equal(sbuff_save(), ALLOCATION_END_POSITION(before, size));
    for (size_t i = 0; i < size; i++) {
        ptr[i] = i + 1;
    }

    void* new_ptr = sbuff_realloc(ptr, 16);
    assert_non_null(ptr);
    assert_ptr_equal(ptr, new_ptr);
    assert_uint_equal(sbuff_save(), ALLOCATION_END_POSITION(before, 20));
    assert_memory_equal(ptr, new_ptr, size);

}

static void test_sbuff_realloc_increase_inplace(void**) {
    size_t before = sbuff_save();
    size_t size = 20;

    char* ptr0 = sbuff_alloc(size);
    assert_non_null(ptr0);
    assert_uint_equal(sbuff_save(), ALLOCATION_END_POSITION(before, size));
    for (size_t i = 0; i < size; i++) {
        ptr0[i] = i + 1;
    }

    void* new_ptr = sbuff_realloc(ptr0, 64);
    assert_non_null(new_ptr);
    assert_ptr_equal(ptr0, new_ptr);
    assert_uint_equal(sbuff_save(), ALLOCATION_END_POSITION(before, size));
    assert_memory_equal(ptr0, new_ptr, size);
    
}

static void test_sbuff_realloc_increase(void**) {
    size_t before0 = sbuff_save();
    
    size_t size0 = 20;
    char* ptr0 = sbuff_alloc(size0);
    assert_non_null(ptr0);
    assert_uint_equal(sbuff_save(), ALLOCATION_END_POSITION(before0, size0));
    for(size_t i = 0; i < size0; i++) {
        ptr0[i] = i + 1;
    }
    
    size_t size1 = 12;
    size_t before1 = sbuff_save();
    void* ptr1 = sbuff_alloc(size1);
    assert_non_null(ptr1);
    assert_uint_equal(sbuff_save(), ALLOCATION_END_POSITION(before1, size1));

    size_t size2 = 64;
    size_t before2 = sbuff_save();
    void* new_ptr = sbuff_realloc(ptr0, size2);
    assert_non_null(new_ptr);
    assert_ptr_not_equal(ptr0, new_ptr);
    assert_uint_equal(sbuff_save(), ALLOCATION_END_POSITION(before2, size2));
    assert_memory_equal(ptr0, new_ptr, size0);

}

static void test_sbuff_reuse(void**) {
    size_t start = sbuff_save();
    size_t size = 40;
    void* ptr = sbuff_alloc(size);
    assert_non_null(ptr);
    assert_uint_equal(sbuff_save(), ALLOCATION_END_POSITION(start, size));

    size_t assert_post = sbuff_save();
    ptr = sbuff_alloc(size);

    size_t before = sbuff_save();

    ptr = sbuff_alloc(size);
    ptr = sbuff_alloc(size);

    sbuff_rewind(before);

    assert_int_equal(sbuff_save(), ALLOCATION_END_POSITION(assert_post, size));
    
}

static int verify(void**) {
    assert_uint_equal(sbuff_save(), sizeof(arena_t));
    return 0;
}

static int reset(void**) {
    sbuff_reset();
    return 0;
}

#define TEST_CASE(testfn) cmocka_unit_test_setup_teardown(testfn, verify, reset)

int main(void) {
    const struct CMUnitTest tests[] = {
        TEST_CASE(test_sbuff_alloc),
        TEST_CASE(test_sbuff_calloc),
        TEST_CASE(test_sbuff_realloc),
        TEST_CASE(test_sbuff_realloc_decrease),
        TEST_CASE(test_sbuff_realloc_increase_inplace),
        TEST_CASE(test_sbuff_realloc_increase),
        TEST_CASE(test_sbuff_reuse),
    };
 
    return cmocka_run_group_tests(tests, reset, verify);
}
