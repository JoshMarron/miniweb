#ifndef INCLUDED_ROUTER_H
#define INCLUDED_ROUTER_H

#include <stdlib.h>

#define router_add_route(router, route, func) \
    router_add_route_inner(router, route, #func, func)

enum
{
    ROUTE_MAX_LENGTH = 255
};

typedef struct router router_t;
typedef int           routerfunc(void* user_data, char const* const request);

router_t* router_init(void);
void      router_destroy(router_t* router);

int router_add_route_inner(router_t* restrict router,
                           char const         route[static 1],
                           char const         func_name[static 1],
                           routerfunc*        func);

routerfunc* router_get_route_func(router_t const* restrict router,
                                  char const               route[static 1]);
int         router_invoke_route_func(router_t const* restrict router,
                                     char const               route[static 1],
                                     void*                    user_data,
                                     char const* const        request);

#endif // INCLUDED_ROUTER_H
