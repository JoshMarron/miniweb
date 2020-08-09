#include "router.t.h"

#include <router.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <cmocka.h>

struct user_data
{
    uint64_t val_to_change;
};

int test_callback(void* user_data, char const* const request)
{
    struct user_data* data = user_data;
    data->val_to_change    = 123;

    return 0;
}

static void test_router_basic_test(void** state)
{
    struct user_data test_data = {0};

    router_t* router = router_init();
    assert_non_null(router);

    int       rc     = router_add_route(router, "testroute", test_callback);
    assert_int_equal(0, rc);

    routerfunc* func = router_get_route_func(router, "testroute");
    assert_non_null(func);

    rc = func(&test_data, "This is a fake request");
    assert_int_equal(0, rc);
    assert_int_equal(123, test_data.val_to_change);

    test_data.val_to_change = 0;
    rc = router_invoke_route_func(router, "testroute", &test_data,
                                  "This is a fake request");
    assert_int_equal(0, rc);
    assert_int_equal(123, test_data.val_to_change);

    router_destroy(router);
}

int run_router_tests()
{
    struct CMUnitTest tests[] = {
        cmocka_unit_test(test_router_basic_test),
    };

    return cmocka_run_group_tests_name("RouterTests", tests, NULL, NULL);
}
