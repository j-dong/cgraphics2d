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

void hashtable_init(Hashtable *h);
void hashtable_free(Hashtable *h);
void hashtable_insert(Hashtable *h, char *key, void *value);
void *hashtable_get(Hashtable *h, char *key);
void hashtable_remove(Hashtable *h, char *key);

#endif
