#ifndef INCLUDED_MINIWEB_HASH_H
#define INCLUDED_MINIWEB_HASH_H

#include <stdbool.h>
#include <stdlib.h>

typedef struct hash hash_t;

typedef struct hash_iter
{
    bool   started;
    size_t last_hash_val; // This ensures we can go through the linked list
    size_t last_element;
} hash_iter_t;

hash_t* hash_init_string_key(size_t init_size, size_t key_offset);

void hash_destroy(hash_t* table);

// ==== MAIN OPERATIONS ====

int   hash_add(hash_t* table, void* data);
void* hash_find(hash_t* table, void const* key);
bool  hash_del(hash_t* table, void const* key);

void* hash_get_next_element(hash_t const* table, hash_iter_t* iterator);

// ==== STATS ====

size_t hash_get_size(hash_t const* table);

#endif
