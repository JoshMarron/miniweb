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

    uint64_t*     ptr    = handle.data;
    *ptr                 = 42;
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

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(basic_test),
        cmocka_unit_test(test_resize),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
