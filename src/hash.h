#ifndef INCLUDED_MINIWEB_HASH_H
#define INCLUDED_MINIWEB_HASH_H

#include <stdbool.h>
#include <stdlib.h>

typedef struct hash hash_t;

hash_t* hash_init_string_key(size_t init_size, size_t key_offset);

void hash_destroy(hash_t* table);

int   hash_add(hash_t* table, void* data);
void* hash_find(hash_t* table, void const* key);
bool  hash_del(hash_t* table, void const* key);

#endif
