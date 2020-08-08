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
    char        key[256];
    uint64_t    val;
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

int run_hash_tests()
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(add_and_find_single_element),
        cmocka_unit_test(add_and_find_multiple_elements),
        cmocka_unit_test(test_growing_table),
        cmocka_unit_test(test_add_then_del),
        cmocka_unit_test(test_dels_dont_conflict),
    };

    return cmocka_run_group_tests_name("HashTableTests", tests, NULL, NULL);
}
