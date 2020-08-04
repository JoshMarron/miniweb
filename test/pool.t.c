#include <pool.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <cmocka.h>

static void basic_test(void** state)
{
    pool_t* pool = *state;

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

static int setup(void** state)
{
    pool_t* pool = pool_init(sizeof(uint64_t), 8);
    if (!pool)
    {
        return -1;
    }

    *state = pool;
    return 0;
}

static int teardown(void** state)
{
    pool_destroy(*state);

    return 0;
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(basic_test, setup, teardown),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
