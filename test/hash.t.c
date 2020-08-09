#include "hash.t.h"

#include <hash.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <cmocka.h>

struct string_key
{
    char     key[256];
    uint64_t val;
};

static void add_and_find_single_element(void** state)
{
    hash_t* table = hash_init_string_key(64, 0);
    assert_non_null(table);

    struct string_key element = {.key = "Hello", .val = 123};
    int               rc      = hash_add(table, &element);
    assert_int_equal(0, rc);

    struct string_key* find_ptr = hash_find(table, "Hello");
    assert_non_null(find_ptr);
    assert_int_equal(element.val, find_ptr->val);

    hash_destroy(table);
}

static void add_and_find_multiple_elements(void** state)
{
    hash_t* table = hash_init_string_key(64, 0);
    assert_non_null(table);

    struct string_key element1 = {.key = "Hello!", .val = 456};
    struct string_key element2 = {.key = "Goodbye!", .val = 123};
    struct string_key element3 = {.key = "jmhash", .val = 123};

    int rc = hash_add(table, &element1);
    assert_int_equal(0, rc);
    rc = hash_add(table, &element2);
    assert_int_equal(0, rc);
    rc = hash_add(table, &element3);
    assert_int_equal(0, rc);

    struct string_key* find_ptr = hash_find(table, "Hello!");
    assert_non_null(find_ptr);
    assert_int_equal(element1.val, find_ptr->val);

    find_ptr = hash_find(table, "Goodbye!");
    assert_non_null(find_ptr);
    assert_int_equal(element2.val, find_ptr->val);

    find_ptr = hash_find(table, "jmhash");
    assert_non_null(find_ptr);
    assert_int_equal(element3.val, find_ptr->val);

    hash_destroy(table);
}

static void test_growing_table(void** state)
{
    hash_t* table = hash_init_string_key(2, 0);
    assert_non_null(table);

    struct string_key element1 = {.key = "Hello!", .val = 456};
    struct string_key element2 = {.key = "Goodbye!", .val = 123};
    struct string_key element3 = {.key = "jmhash", .val = 123};

    int rc = hash_add(table, &element1);
    assert_int_equal(0, rc);
    rc = hash_add(table, &element2);
    assert_int_equal(0, rc);
    rc = hash_add(table, &element3);
    assert_int_equal(0, rc);

    struct string_key* find_ptr = hash_find(table, "Hello!");
    assert_non_null(find_ptr);
    assert_int_equal(element1.val, find_ptr->val);

    find_ptr = hash_find(table, "Goodbye!");
    assert_non_null(find_ptr);
    assert_int_equal(element2.val, find_ptr->val);

    find_ptr = hash_find(table, "jmhash");
    assert_non_null(find_ptr);
    assert_int_equal(element3.val, find_ptr->val);

    hash_destroy(table);
}

static void test_add_then_del(void** state)
{
    hash_t* table = hash_init_string_key(16, 0);
    assert_non_null(table);

    struct string_key element = {.key = "Hello!", .val = 456};

    int rc = hash_add(table, &element);
    assert_int_equal(0, rc);

    struct string_key* find_ptr = hash_find(table, "Hello!");
    assert_non_null(find_ptr);
    assert_int_equal(element.val, find_ptr->val);

    assert_true(hash_del(table, "Hello!"));

    // Check it was actually deleted
    find_ptr = hash_find(table, "Hello!");
    assert_null(find_ptr);

    hash_destroy(table);
}

static void test_dels_dont_conflict(void** state)
{
    hash_t* table = hash_init_string_key(2, 0);
    assert_non_null(table);

    // These two SHOULD hash to the same bucket value, so we can test
    // the linked list behaviour...
    struct string_key element1 = {.key = "Hello!", .val = 123};
    struct string_key element2 = {.key = "Goodby!", .val = 126};

    int rc = hash_add(table, &element1);
    assert_int_equal(0, rc);
    rc = hash_add(table, &element2);
    assert_int_equal(0, rc);

    // We're at capacity now
    assert_true(hash_del(table, "Hello!"));

    // This element should be gone
    struct string_key* find_ptr = hash_find(table, "Hello!");
    assert_null(find_ptr);

    // This element should not be gone
    find_ptr = hash_find(table, "Goodby!");
    assert_non_null(find_ptr);
    assert_int_equal(find_ptr->val, element2.val);

    hash_destroy(table);
}

static void test_hash_size_tracked_correctly(void** state)
{
    hash_t* table = hash_init_string_key(16, 0);
    assert_non_null(table);

    struct string_key element1 = {.key = "Hello!", .val = 123};
    struct string_key element2 = {.key = "Goodbye!", .val = 126};

    int rc = hash_add(table, &element1);
    assert_int_equal(0, rc);
    rc = hash_add(table, &element2);
    assert_int_equal(0, rc);

    hash_destroy(table);
}

static void test_basic_iteration(void** state)
{
    hash_t* table = hash_init_string_key(16, 0);
    assert_non_null(table);

    struct string_key elements[] = {{"Hello!", 123},
                                    {"Goodbye!", 127},
                                    {"jmhash", 444},
                                    {"a test val", 777},
                                    {"me gusta", 778}};

    for (size_t i = 0; i < sizeof(elements) / sizeof(struct string_key); ++i)
    {
        int rc = hash_add(table, &elements[i]);
        assert_int_equal(0, rc);
    }

    for (size_t i = 0; i < sizeof(elements) / sizeof(struct string_key); ++i)
    {
        struct string_key* find_ptr = hash_find(table, &elements[i]);
        assert_non_null(find_ptr);
    }

    uint64_t           seen_vals[sizeof(elements) / sizeof(struct string_key)] = {0};
    hash_iter_t        iterator                                                = {0};
    struct string_key* iter_ptr   = NULL;
    size_t             vals_found = 0;
    while ((iter_ptr = hash_get_next_element(table, &iterator)))
    {
        uint64_t val = iter_ptr->val;
        // Check we haven't found our value yet
        for (size_t i = 0; i < vals_found; ++i)
        {
            assert_int_not_equal(seen_vals[i], val);
        }

        seen_vals[vals_found] = val;
        ++vals_found;
    }

    // Check we found all the elements!
    assert_int_equal(vals_found, (sizeof(elements) / sizeof(struct string_key)));

    hash_destroy(table);
}

void test_iteration_in_chain(void** state)
{
    hash_t* table = hash_init_string_key(2, 0);
    assert_non_null(table);

    struct string_key elements[] = {{"Hello!", 456}, {"Goodby!", 123}};

    for (size_t i = 0; i < 2; ++i)
    {
        int rc = hash_add(table, &elements[i]);
        assert_int_equal(0, rc);
    }

    uint64_t           seen_vals[sizeof(elements) / sizeof(struct string_key)] = {0};
    hash_iter_t        iterator                                                = {0};
    struct string_key* iter_ptr   = NULL;
    size_t             vals_found = 0;
    while ((iter_ptr = hash_get_next_element(table, &iterator)))
    {
        uint64_t val = iter_ptr->val;
        // Check we haven't found our value yet
        for (size_t i = 0; i < vals_found; ++i)
        {
            assert_int_not_equal(seen_vals[i], val);
        }

        seen_vals[vals_found] = val;
        ++vals_found;
    }

    // Check we found all the elements!
    assert_int_equal(vals_found, (sizeof(elements) / sizeof(struct string_key)));

    hash_destroy(table);
}

int run_hash_tests()
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(add_and_find_single_element),
        cmocka_unit_test(add_and_find_multiple_elements),
        cmocka_unit_test(test_growing_table),
        cmocka_unit_test(test_add_then_del),
        cmocka_unit_test(test_dels_dont_conflict),
        cmocka_unit_test(test_hash_size_tracked_correctly),
        cmocka_unit_test(test_basic_iteration),
        cmocka_unit_test(test_iteration_in_chain),
    };

    return cmocka_run_group_tests_name("HashTableTests", tests, NULL, NULL);
}
