#ifndef INCLUDED_MINIWEB_LOGGING_H
#define INCLUDED_MINIWEB_LOGGING_H

#include <stdbool.h>
#include <stdio.h>

#define STRGY(X)     #X
#define STRINGIFY(X) STRGY(X)

#define GET_NUM_ARGS(...)                                                          \
    SELECT_LAST(__VA_ARGS__, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20,   \
                19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, \
                0)

#define SELECT_LAST(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, \
                    a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, \
                    a26, a27, a28, a29, a30, a31, a32, ...)                     \
    a32

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
            printf("%s (line " STRINGIFY(__LINE__) "): " EXPR "\n", __func__);    \
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
            fprintf(stderr, "%s (line " STRINGIFY(__LINE__) "): " EXPR "\n",     \
                    __func__);                                                   \
        }                                                                        \
    } while (false)

#endif // INCLUDED_MINIWEB_LOGGING_H
