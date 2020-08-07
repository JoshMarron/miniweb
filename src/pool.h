#ifndef INCLUDED_POOL_H
#define INCLUDED_POOL_H

#include <stdlib.h>

// An extremely simple pool allocator with fixed-size blocks. Should only really
// be used if you know specifically what you're going to allocate in it!

typedef struct pool pool_t;
typedef size_t      pool_id_t;

typedef struct pool_handle
{
    pool_id_t id;
    void*     data;
} pool_handle_t;

// CREATORS AND DESTROYERS
pool_t* pool_init(size_t block_size, size_t init_num_blocks);
int pool_create(pool_t* restrict pool, size_t block_size, size_t init_num_blocks);
// Will free everything in the pool
void pool_clear(pool_t* restrict pool);
void pool_destroy(pool_t* restrict pool);

pool_handle_t pool_alloc(pool_t* pool);
pool_handle_t pool_calloc(pool_t* pool);

// Can we do pointer comparisons to find which control block I belong to with binary
// search?
void pool_free(pool_t* pool, pool_handle_t handle);

#endif // INCLUDED_POOL_H
