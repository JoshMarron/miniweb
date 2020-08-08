#include "router.h"

struct router
{};

struct route
{
    char const* route_name;
    routerfunc* func;
};
