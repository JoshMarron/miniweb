#ifndef INCLUDED_ROUTER_H
#define INCLUDED_ROUTER_H

#include "miniweb_response.h"

#include <stdlib.h>

#define router_add_route(router, route, func, user_data) \
    router_add_route_inner(router, route, #func, func, user_data)

enum
{
    ROUTE_MAX_LENGTH = 255
};

typedef struct router      router_t;
typedef miniweb_response_t routerfunc(void* user_data, char const* const request);

router_t* router_init(void);
void      router_destroy(router_t* restrict router);

int router_add_route_inner(router_t* restrict router,
                           char const         route[static 1],
                           char const         func_name[static 1],
                           routerfunc*        func,
                           void*              user_data);

routerfunc*        router_get_route_func(router_t const* restrict router,
                                         char const               route[static 1],
                                         void**                   user_data);
miniweb_response_t router_invoke_route_func(router_t const* restrict router,
                                            char const               route[static 1],
                                            char const* const        request);

#endif // INCLUDED_ROUTER_H
