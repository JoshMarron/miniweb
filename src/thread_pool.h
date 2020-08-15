#ifndef INCLUDED_THREAD_POOL_H
#define INCLUDED_THREAD_POOL_H

#include <stdlib.h>

typedef struct thread_pool thread_pool_t;
typedef void thread_pool_func(void*);

thread_pool_t* thread_pool_init(size_t num_threads);
void           thread_pool_destroy(thread_pool_t* restrict pool);

int thread_pool_run(thread_pool_t* restrict pool,
                    thread_pool_func*       func,
                    void*                   data);

#endif // INCLUDED_THREAD_POOL_H
