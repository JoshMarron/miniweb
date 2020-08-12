#include "router.t.h"

#include <miniweb_response.h>
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

miniweb_response_t test_callback(void* user_data, char const* const request)
{
    struct user_data* data = user_data;
    data->val_to_change    = 123;

    return miniweb_build_text_response("Hello!");
}

static void test_router_basic_test(void** state)
{
    struct user_data test_data = {0};

    router_t* router = router_init();
    assert_non_null(router);

    int rc = router_add_route(router, "testroute", test_callback, &test_data);
    assert_int_equal(0, rc);

    routerfunc* func = router_get_route_func(router, "testroute");
    assert_non_null(func);

    miniweb_response_t response = func(&test_data, "This is a fake request");
    assert_string_equal("Hello!", response.body.text_response.text_response);
    assert_int_equal(123, test_data.val_to_change);

    test_data.val_to_change = 0;
    response =
        router_invoke_route_func(router, "testroute", "This is a fake request");
    assert_string_equal("Hello!", response.body.text_response.text_response);
    assert_int_equal(123, test_data.val_to_change);

    router_destroy(router);
}

static void test_returns_null_on_route_not_find(void** state)
{
    router_t* router = router_init();
    assert_non_null(router);

    routerfunc* func = router_get_route_func(router, "notfoundroute");
    assert_null(func);

    miniweb_response_t response =
        router_invoke_route_func(router, "notfoundroute2", "This is a fake request");
    assert_string_equal("404", response.body.text_response.text_response);

    router_destroy(router);
}

int run_router_tests()
{
    struct CMUnitTest tests[] = {
        cmocka_unit_test(test_router_basic_test),
        cmocka_unit_test(test_returns_null_on_route_not_find),
    };

    return cmocka_run_group_tests_name("RouterTests", tests, NULL, NULL);
}
