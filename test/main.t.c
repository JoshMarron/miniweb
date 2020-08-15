#include "hash.t.h"
#include "pool.t.h"
#include "router.t.h"
#include "thread_pool.t.h"

int main(void)
{
    int rc = 0;
    rc |= run_pool_tests();
    rc |= run_hash_tests();
    rc |= run_router_tests();
    rc |= run_thread_pool_tests();

    return rc;
}
