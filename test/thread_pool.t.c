#include "thread_pool.t.h"

#include <thread_pool.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <cmocka.h>

void basic_test_func(void* data)
{
    atomic_int* cast_data = data;
    *cast_data          = 1234;
}

static void basic_test(void** state)
{
    thread_pool_t* pool = thread_pool_init(2);
    assert_non_null(pool);

    atomic_int my_data = 0;
    thread_pool_run(pool, &basic_test_func, &my_data);

    thread_pool_destroy(pool);

    // The job should definitely be finished now, so let's check the data
    int result = my_data;
    assert_int_equal(1234, result);
}

int run_thread_pool_tests()
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(basic_test),
    };

    return cmocka_run_group_tests_name("ThreadPoolTests", tests, NULL, NULL);
}
