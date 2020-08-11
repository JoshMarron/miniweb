#include "router.h"

#include "hash.h"
#include "logging.h"
#include "pool.h"

#include <stddef.h>
#include <string.h>

#ifdef MINIWEB_TESTING
extern void* _test_malloc(const size_t size, char const* file, int const line);
extern void*
             _test_calloc(size_t nmemb, size_t size, char const* file, int const line);
extern void  _test_free(void* ptr, char const* file, int const line);
extern void* _test_realloc(void* ptr, size_t size, char const* file, int const line);

    #define malloc(size)       _test_malloc(size, __FILE__, __LINE__)
    #define calloc(n, size)    _test_calloc(n, size, __FILE__, __LINE__)
    #define free(ptr)          _test_free(ptr, __FILE__, __LINE__)
    #define realloc(ptr, size) _test_realloc(ptr, size, __FILE__, __LINE__)
#endif

struct router
{
    // We'll allocate the routes with this
    pool_t* route_pool;
    hash_t* route_table;
};

struct route
{
    char          route_name[ROUTE_MAX_LENGTH + 1];
    routerfunc* func;
    pool_handle_t handle; // Self-referential handle
};

// ==== CONSTANTS ====

static const size_t DEFAULT_ROUTE_POOL_SIZE = 16;
static const size_t DEFAULT_ROUTE_HASH_SIZE = 16;

struct router* router_init(void)
{
    pool_t* route_pool = pool_init(sizeof(struct route), DEFAULT_ROUTE_POOL_SIZE);
    if (!route_pool)
    {
        MINIWEB_LOG_ERROR("Failed to create route_pool");
        return NULL;
    }

    hash_t* route_hash = hash_init_string_key(DEFAULT_ROUTE_HASH_SIZE,
                                              offsetof(struct route, route_name));
    if (!route_hash)
    {
        MINIWEB_LOG_ERROR("Failed to create route_hash!");
        pool_destroy(route_pool);
        return NULL;
    }

    struct router* router = calloc(1, sizeof(struct router));
    if (!router)
    {
        MINIWEB_LOG_ERROR("Failed to allocate space for router!");
        pool_destroy(route_pool);
        hash_destroy(route_hash);
        return NULL;
    }

    router->route_pool  = route_pool;
    router->route_table = route_hash;

    return router;
}

void router_destroy(router_t* restrict router)
{
    hash_destroy(router->route_table);
    // This will free all the routes
    pool_destroy(router->route_pool);
    free(router);
}

int router_add_route_inner(struct router* restrict router,
                           char const              route[static 1],
                           char const              func_name[static 1],
                           routerfunc*             func)
{
    pool_handle_t new_route_handle = pool_alloc(router->route_pool);
    struct route* new_route        = new_route_handle.data;
    if (!new_route)
    {
        MINIWEB_LOG_ERROR("Failed to add route %s, pool_alloc failed", route);
        return -1;
    }

    // Copy in the route name
    strncpy(new_route->route_name, route, ROUTE_MAX_LENGTH);
    new_route->route_name[ROUTE_MAX_LENGTH] = '\0';

    new_route->func   = func;
    new_route->handle = new_route_handle;

    int rc = hash_add(router->route_table, new_route);
    if (rc != 0)
    {
        MINIWEB_LOG_ERROR("Failed to add route %s, hash_add failed, rc %d", route,
                          rc);
        return -2;
    }

    MINIWEB_LOG_INFO("Added route %s, will call func name %s", route, func_name);

    return 0;
}

routerfunc* router_get_route_func(struct router const* restrict router,
                                  char const                    route[static 1])
{
    struct route* found_route = hash_find(router->route_table, route);
    if (!found_route)
    {
        MINIWEB_LOG_ERROR("Could not find route %s in router_table", route);
        return NULL;
    }

    return found_route->func;
}

miniweb_response_t router_invoke_route_func(struct router const* restrict router,
                                            char const        route[static 1],
                                            void*             user_data,
                                            char const* const request)
{
    struct route* found_route = hash_find(router->route_table, route);
    if (!found_route)
    {
        MINIWEB_LOG_ERROR("Could not find route %s in router_table", route);
        // TODO: Make this return a proper 404
        return miniweb_build_text_response("404");
    }

    return found_route->func(user_data, request);
}
