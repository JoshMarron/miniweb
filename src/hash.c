#include "hash.h"

#include "logging.h"
#include "pool.h"

#include <assert.h>
#include <stdint.h>

#ifdef MINIWEB_TESTING
extern void* _test_malloc(const size_t size, char const* file, int const line);
extern void*
             _test_calloc(size_t nmemb, size_t size, char const* file, int const line);
extern void  _test_free(void* ptr, char const* file, int const line);
extern void* _test_realloc(void* ptr, size_t size, char const* file, int const line);

    #define malloc(size)       _test_malloc(size, __FILE__, __LINE__)
    #define calloc(n, size)    _test_calloc(n, size, __FILE__, __LINE__)
    #define free(ptr)          _test_free(ptr, __FILE__, __LINE__)
    #define realloc(ptr, size) _test_realloc(ptr, size, __FILE__, __LINE__)
#endif

// ==== CONSTANTS ====

static const double MAX_LOAD_FACTOR = 0.8;

// ==== TYPES ====

typedef size_t hashfunc(void const* data, size_t key_len);

struct hash_entry
{
    void*  data;
    size_t hash_val;
    // So that we can make the linked list of entries
    pool_handle_t next;
};

struct hash
{
    // We'll store the pointers in here, indexed by the hash
    pool_handle_t* storage;

    size_t key_offset;
    size_t key_len;
    size_t storage_size;
    size_t num_entries;

    hashfunc* do_hash;
    pool_t*   entry_pool;
};

// ==== STATIC FUNCTIONS ====

static size_t djb2_hash_str(void const* data, size_t key_len)
{
    // We can ignore this parameter
    (void) key_len;

    unsigned char const* str  = data;
    int                  c    = *str;
    size_t               hash = 5381;

    while (c != 0)
    {
        hash = ((hash << 5) + hash) + c; // equivalent to hash * 33 + c
        ++str;
        c = *str;
    }

    return hash;
}

static int hash_resize(struct hash* table)
{
    // We need to:
    // Allocate new entry space, twice as big as before
    // Rehash every element into the new space
    // Free the old space

    size_t         new_storage_size = table->storage_size * 2;
    pool_handle_t* storage = calloc(new_storage_size, sizeof(pool_handle_t));
    if (!storage)
    {
        MINIWEB_LOG_ERROR("Could not alloc %zu bytes for hash table expansion",
                          new_storage_size * sizeof(pool_handle_t));
        return -1;
    }

    size_t num_rehashes = 0;
    for (size_t i = 0; i < table->storage_size; ++i)
    {
        pool_handle_t this_handle = table->storage[i];
        while (this_handle.data)
        {
            struct hash_entry*            entry    = this_handle.data;
            unsigned char const* restrict data_ptr = entry->data;
            size_t                        hash =
                table->do_hash(data_ptr + table->key_offset, table->key_len);
            size_t new_bucket = hash % new_storage_size;

            pool_handle_t old_next = entry->next;

            entry->hash_val     = hash;
            entry->next         = storage[new_bucket];
            storage[new_bucket] = this_handle;

            this_handle = old_next;
            ++num_rehashes;
        }
    }

    MINIWEB_LOG_INFO("Done rehash. %zu elements rehashed", num_rehashes);

    table->storage_size = new_storage_size;
    free(table->storage);
    table->storage = storage;

    return 0;
}

// ==== PUBLIC FUNCTION IMPLEMENTATIONS ====

struct hash* hash_init_string_key(size_t init_size, size_t key_offset)
{
    // Try to allocate our initial storage first
    pool_handle_t* storage = calloc(init_size, sizeof(pool_handle_t));
    if (!storage)
    {
        MINIWEB_LOG_ERROR("Failed to allocate %zu bytes for initial storage",
                          sizeof(struct hash_entry) * init_size);
        return NULL;
    }

    pool_t* pool = pool_init(sizeof(struct hash_entry), init_size);
    if (!pool)
    {
        MINIWEB_LOG_ERROR("Failed to create hash_entry pool!");
        free(storage);
        return NULL;
    }

    struct hash* table = calloc(1, sizeof(struct hash));
    if (!table)
    {
        MINIWEB_LOG_ERROR("Failed to allocate space for the table metadata");
        free(storage);
        pool_destroy(pool);
        return NULL;
    }

    table->storage    = storage;
    table->key_offset = key_offset;
    table->key_len    = SIZE_MAX;
    table->storage_size = init_size;
    table->num_entries  = 0;
    table->do_hash      = &djb2_hash_str;
    table->entry_pool   = pool;

    return table;
}

int hash_add(struct hash* table, void* data)
{
    assert(table);
    assert(data);

    if (((double) table->num_entries / (double) table->storage_size) >
        MAX_LOAD_FACTOR)
    {
        hash_resize(table);
    }

    unsigned char const* restrict data_p = data;
    size_t hash_val = table->do_hash(data_p + table->key_offset, table->key_len);
    size_t bucket   = hash_val % table->storage_size;

    // Create a new hash_entry
    pool_handle_t new_handle = pool_alloc(table->entry_pool);
    if (!new_handle.data)
    {
        MINIWEB_LOG_ERROR("Failed to allocate new hash entry!");
        return -1;
    }

    pool_handle_t* current_entry = &(table->storage[bucket]);

    struct hash_entry* new_entry = new_handle.data;
    new_entry->data              = data;
    new_entry->hash_val          = hash_val;
    new_entry->next              = *current_entry;

    table->storage[bucket] = new_handle;
    ++table->num_entries;

    return 0;
}

void* hash_find(hash_t* table, void const* key)
{
    assert(table);
    assert(key);

    unsigned char const* restrict data_p = key;
    size_t hash_val = table->do_hash(data_p + table->key_offset, table->key_len);
    size_t bucket   = hash_val % table->storage_size;

    pool_handle_t* handle_entry = &(table->storage[bucket]);
    while (handle_entry->data)
    {
        struct hash_entry* entry = handle_entry->data;
        if (entry->hash_val == hash_val) { return entry->data; }

        handle_entry = &entry->next;
    }

    // If we get here, then we didn't find it
    return NULL;
}

bool hash_del(hash_t* table, void const* key)
{
    assert(table);
    assert(key);

    unsigned char const* restrict data_p = key;
    size_t hash_val = table->do_hash(data_p + table->key_offset, table->key_len);
    size_t bucket   = hash_val % table->storage_size;

    pool_handle_t* handle_entry = &(table->storage[bucket]);
    while (handle_entry->data)
    {
        struct hash_entry* entry = handle_entry->data;
        if (entry->hash_val == hash_val)
        {
            // Delete the node
            return true;
        }

        handle_entry = &entry->next;
    }

    return false;
}

void hash_destroy(struct hash* table)
{
    assert(table);

    pool_destroy(table->entry_pool);
    free(table->storage);
    free(table);
}
