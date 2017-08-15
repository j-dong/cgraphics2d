#ifndef INCLUDED_GRAPHICS_HASHTABLE_H
#define INCLUDED_GRAPHICS_HASHTABLE_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

// robin-hood hash table for strings
// see https://www.sebastiansylvan.com/post/robin-hood-hashing-should-be-your-default-hash-table-implementation/
// and http://codecapsule.com/2013/11/17/robin-hood-hashing-backward-shift-deletion/

// if key is NULL then empty
struct hashtable_t {
    // capacity is always a power of 2
    // initial capacity is specified in hashtable.c
    size_t data_len,
           data_cap,
           resize_cap;
    // mask = capacity - 1
    // so we can do a bitwise AND
    uint64_t mask;
    struct hashtable_entry_t {
        char *key;
        void *value;
        uint64_t hash;
    } *data;
};

typedef struct hashtable_t Hashtable;

// Initialize a hashtable.
// Initial capacity is specified in hashtable.c
void hashtable_init(Hashtable *h);
// Free a hashtable.
// Does not free elements.
void hashtable_free(Hashtable *h);
// Returns pointer to value, so you can choose what to set it to
// based on the previous value. Value will be set to NULL if
// newly inserted.
// Copies key.
void **hashtable_ready_put(Hashtable *h, const char *key);
// Returns old value if occupied (does overwrite).
// Returns NULL if not occupied.
// Copies key.
inline void *hashtable_put(Hashtable *h, const char *key, void *value) {
    void **value_ptr = hashtable_ready_put(h, key);
    void *ret = *value_ptr;
    *value_ptr = value;
    return ret;
}
// Returns NULL if not there.
void *hashtable_get(Hashtable *h, const char *key);
// Returns element removed, or NULL.
void *hashtable_remove(Hashtable *h, const char *key);

inline void hashtable_traverse(Hashtable *h, void (*callback)(const char *key, void *value)) {
    for (size_t i = 0; i < h->data_cap; i++)
        if (h->data[i].key)
            callback(h->data[i].key, h->data[i].value);
}

inline void hashtable_traverse_data(Hashtable *h, void *data, void (*callback)(void *data, const char *key, void *value)) {
    for (size_t i = 0; i < h->data_cap; i++)
        if (h->data[i].key)
            callback(data, h->data[i].key, h->data[i].value);
}

inline void hashtable_traverse_values(Hashtable *h, void (*callback)(void *value)) {
    for (size_t i = 0; i < h->data_cap; i++)
        if (h->data[i].key)
            callback(h->data[i].value);
}

#endif
