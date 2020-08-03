#ifndef INCLUDED_MINIWEB_LOGGING_H
#define INCLUDED_MINIWEB_LOGGING_H

#include "macro_helpers.h"

#include <stdbool.h>
#include <stdio.h>

#define MINIWEB_LOG_INFO(...) \
    MINIWEB_LOG_INFO_INNER(GET_NUM_ARGS(__VA_ARGS__), __VA_ARGS__, 0)

#define MINIWEB_LOG_ERROR(...) \
    MINIWEB_LOG_ERROR_INNER(GET_NUM_ARGS(__VA_ARGS__), __VA_ARGS__, 0)

#define MINIWEB_LOG_INFO_INNER(NARGS, EXPR, ...)                                  \
    do                                                                            \
    {                                                                             \
        if (NARGS > 1)                                                            \
        {                                                                         \
            printf("%s (line " STRINGIFY(__LINE__) "): " EXPR "%0.d\n", __func__, \
                   __VA_ARGS__);                                                  \
        }                                                                         \
        else                                                                      \
        {                                                                         \
            printf("%s (line " STRINGIFY(__LINE__) "): %s\n", __func__, EXPR);    \
        }                                                                         \
    } while (false)

#define MINIWEB_LOG_ERROR_INNER(NARGS, EXPR, ...)                                \
    do                                                                           \
    {                                                                            \
        if (NARGS > 1)                                                           \
        {                                                                        \
            fprintf(stderr, "%s (line " STRINGIFY(__LINE__) "): " EXPR "%0.d\n", \
                    __func__, __VA_ARGS__);                                      \
        }                                                                        \
        else                                                                     \
        {                                                                        \
            fprintf(stderr, "%s (line " STRINGIFY(__LINE__) "): %s\n", __func__, \
                    EXPR);                                                       \
        }                                                                        \
    } while (false)

#endif // INCLUDED_MINIWEB_LOGGING_H
