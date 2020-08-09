#include "pool.t.h"

#include <pool.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <cmocka.h>

struct pool_test_state
{
    pool_t* pool;
};

static void basic_test(void** state)
{
    pool_t* pool = pool_init(sizeof(uint64_t), 8);

    pool_handle_t handle = pool_alloc(pool);
    size_t        id     = handle.id;

    uint64_t* ptr = handle.data;
    *ptr          = 42;
    pool_free(pool, handle);

    // We should get the same block back
    handle = pool_alloc(pool);
    assert_int_equal(id, handle.id);

    pool_free(pool, handle);
    pool_destroy(pool);
}

static void test_resize(void** state)
{
    pool_t* pool = pool_init(sizeof(uint64_t), 8);

    for (size_t i = 0; i < 9; ++i)
    {
        pool_handle_t handle = pool_alloc(pool);
        assert_non_null(handle.data);
    }

    // Should have triggered a resize, so let's alloc a new handle and try to set it
    pool_handle_t handle = pool_alloc(pool);
    assert_int_equal(9, handle.id);
    assert_non_null(handle.data);
    uint64_t* ptr = handle.data;
    *ptr          = 42;

    pool_free(pool, handle);
    pool_destroy(pool);
}

static void test_pool_calloc(void** state)
{
    pool_t* pool = pool_init(sizeof(uint64_t), 8);

    pool_handle_t handle = pool_calloc(pool);
    uint64_t*     ptr    = handle.data;

    // The calloc'd pointer should point to all 0s, which for an unsigned,
    // should obviously be 0
    assert_non_null(ptr);
    assert_int_equal(*ptr, 0);

    pool_destroy(pool);
}

static void test_big_pool(void** state)
{
    pool_t* pool = pool_init(sizeof(uint64_t), 1000);

    for (size_t i = 0; i < 1000000; ++i)
    {
        pool_handle_t handle = pool_alloc(pool);
        uint64_t*     ptr    = handle.data;
        assert_non_null(ptr);
        *ptr = i;
    }

    // All the memory should be freed here
    pool_destroy(pool);
}

static void test_reallocating_over_and_over(void** state)
{
    pool_t* pool = pool_init(sizeof(uint64_t), 1);

    // If we alloc and free the same block over and over the pool should never resize
    for (size_t i = 0; i < 50000; ++i)
    {
        pool_handle_t handle = pool_alloc(pool);
        assert_non_null(handle.data);
        assert_int_equal(0, handle.id);
        pool_free(pool, handle);
    }

    pool_destroy(pool);
}

int run_pool_tests()
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(basic_test),
        cmocka_unit_test(test_resize),
        cmocka_unit_test(test_pool_calloc),
        cmocka_unit_test(test_reallocating_over_and_over),
        cmocka_unit_test(test_big_pool),
    };

    return cmocka_run_group_tests_name("PoolTests", tests, NULL, NULL);
}
