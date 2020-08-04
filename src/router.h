#ifndef INCLUDED_ROUTER_H
#define INCLUDED_ROUTER_H

#include <stdlib.h>

typedef struct router router_t;

int router_add_route(size_t routelen, char const route[routelen]);

#endif // INCLUDED_ROUTER_H
