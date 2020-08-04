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
    struct pool_test_state* pool_state = *state;
    pool_t*                pool       = pool_state->pool;

    pool_handle_t handle = pool_alloc(pool);
    size_t        id     = handle.id;

    uint64_t*     ptr    = handle.data;
    *ptr                 = 42;
    pool_free(pool, handle);

    // We should get the same block back
    handle = pool_alloc(pool);
    assert_int_equal(id, handle.id);

    pool_free(pool, handle);
}

static void test_resize(void** state)
{
    struct pool_test_state* pool_state = *state;
    pool_t*                 pool       = pool_state->pool;

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
}

static int setup(void** state)
{
    pool_t* pool = pool_init(sizeof(uint64_t), 8);
    if (!pool)
    {
        return -1;
    }

    struct pool_test_state* pool_state = calloc(1, sizeof(struct pool_test_state));
    if (!pool_state)
    {
        pool_destroy(pool);
        return -1;
    }

    pool_state->pool = pool;

    *state = pool_state;
    return 0;
}

static int teardown(void** state)
{
    struct pool_test_state* pool_state = *state;
    pool_destroy(pool_state->pool);
    free(pool_state);

    return 0;
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(basic_test, setup, teardown),
        cmocka_unit_test_setup_teardown(test_resize, setup, teardown),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
