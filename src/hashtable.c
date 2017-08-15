#include "hashtable.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

// forward declaration of hash function

#define HASHTABLE_HASH_SEED 0

static inline uint64_t str_hash(const char *key, const size_t len);

// hash table implementation

#define HASHTABLE_INITIAL_CAPACITY 8
#define HASHTABLE_LOAD_FACTOR_PERCENT 80

void hashtable_init(Hashtable *h) {
    h->data_len = 0;
    h->data_cap = HASHTABLE_INITIAL_CAPACITY;
    h->resize_cap = h->data_cap * HASHTABLE_LOAD_FACTOR_PERCENT / 100;
    h->mask = h->data_cap - 1;
    h->data = calloc(h->data_cap, sizeof *h->data);
}

void hashtable_free(Hashtable *h) {
    for (size_t i = 0; i < h->data_cap; i++)
        if (h->data[i].key)
            free(h->data[i].key);
    free(h->data);
}

inline static size_t hashtable_dib(Hashtable *h, size_t pos, uint64_t hash) {
    return (pos - hash) & h->mask;
}

inline static void **hashtable_ready_put_impl(Hashtable *h, char *key, size_t len, uint64_t hash, bool copy_key, bool check_duplicate) {
    if (copy_key) {
        char *old_key = key;
        key = malloc(len + 1);
        memcpy(key, old_key, len + 1);
    }
    size_t pos = hash & h->mask;
    size_t dib = 0;
    void **ret = NULL;
    void *value = NULL;
    while (h->data[pos].key) {
        if (check_duplicate
         && h->data[pos].hash == hash
         && strcmp(h->data[pos].key, key) == 0) {
            if (copy_key)
                free(key);
            return &h->data[pos].value;
        }
        size_t cur_dib = hashtable_dib(h, pos, h->data[pos].hash);
        if (cur_dib < dib) {
            // swap elements to insert
            char *t_key = h->data[pos].key;
            void *t_value = h->data[pos].value;
            uint64_t t_hash = h->data[pos].hash;
            h->data[pos].key = key;
            h->data[pos].value = value;
            h->data[pos].hash = hash;
            key = t_key;
            value = t_value;
            hash = t_hash;
            dib = cur_dib;
            check_duplicate = false;
            if (!ret) ret = &h->data[pos].value;
        }
        dib++;
        pos = (pos + 1) & h->mask;
    }
    h->data_len++;
    h->data[pos].key = key;
    h->data[pos].value = value;
    h->data[pos].hash = hash;
    if (!ret) ret = &h->data[pos].value;
    return ret;
}

static void hashtable_resize(Hashtable *h) {
    size_t old_cap = h->data_cap;
    size_t old_len = h->data_len;
    struct hashtable_entry_t *old_data = h->data;
    h->data_cap = old_cap * 2;
    h->resize_cap = h->data_cap * HASHTABLE_LOAD_FACTOR_PERCENT / 100;
    h->mask = h->data_cap - 1;
    h->data = calloc(h->data_cap, sizeof *h->data);
    // copy old data
    h->data_len = 0;
    for (size_t i = 0; i < old_cap; i++) {
        if (old_data[i].key) {
            // we know that there are no duplicates, and we can
            // safely pass the key without copying
            *hashtable_ready_put_impl(h,
                old_data[i].key, strlen(old_data[i].key), old_data[i].hash,
                false, false) = old_data[i].value;
        }
    }
    free(old_data);
    assert(h->data_len == old_len);
}

void **hashtable_ready_put(Hashtable *h, char *key) {
    if (h->data_len >= h->resize_cap)
        hashtable_resize(h);
    size_t len = strlen(key);
    uint64_t hash = str_hash(key, len);
    return hashtable_ready_put_impl(h, key, len, hash, true, true);
}

void *hashtable_put(Hashtable *h, char *key, void *value);

void *hashtable_get(Hashtable *h, char *key) {
    size_t dib = 0;
    size_t len = strlen(key);
    uint64_t hash = str_hash(key, len);
    size_t pos = hash & h->mask;
    while (h->data[pos].key) {
        if (h->data[pos].hash == hash
         && strcmp(h->data[pos].key, key) == 0) {
            return h->data[pos].value;
        }
        size_t cur_dib = hashtable_dib(h, pos, h->data[pos].hash);
        if (cur_dib < dib) {
            break;
        }
        dib++;
        pos = (pos + 1) & h->mask;
    }
    return NULL;
}

void *hashtable_remove(Hashtable *h, char *key) {
    // search for element
    size_t dib = 0;
    size_t len = strlen(key);
    uint64_t hash = str_hash(key, len);
    size_t pos = hash & h->mask;
    void *ret;
    while (true) {
        if (!h->data[pos].key) {
            return NULL;
        }
        if (h->data[pos].hash == hash
         && strcmp(h->data[pos].key, key) == 0) {
            ret = h->data[pos].value;
            break;
        }
        size_t cur_dib = hashtable_dib(h, pos, h->data[pos].hash);
        if (cur_dib < dib) {
            return NULL;
        }
        dib++;
        pos = (pos + 1) & h->mask;
    }
    // pos holds the index of entry to delete
    // shift following elements backwards
    // until empty or 0 DIB
    size_t next_pos;
    while (true) {
        next_pos = (pos + 1) & h->mask;
        if (!h->data[next_pos].key)
            break;
        if (hashtable_dib(h, next_pos, h->data[next_pos].hash) == 0)
            break;
        // would like to do a bulk memmove, but it'd be annoying
        // to deal with wrapping around
        memcpy(&h->data[pos], &h->data[next_pos], sizeof(struct hashtable_entry_t));
        pos = next_pos;
    }
    h->data[pos].key = NULL;
    h->data_len--;
    return ret;
}

void hashtable_traverse(Hashtable *h, void (*callback)(char *key, void *value));
void hashtable_traverse_data(Hashtable *h, void *data, void (*callback)(void *data, char *key, void *value));
void hashtable_traverse_values(Hashtable *h, void (*callback)(void *value));

// hash function

// hash function makes assumptions about target endianness
#if defined(_MSC_VER)
#if !defined(_M_X64)
#error "MurmurHash3 is optimized for x86-64 systems."
#endif
#elif defined(__GNUC__)
#if !defined(__x86_64__)
#error "MurmurHash3 is optimized for x86-64 systems."
#endif
#else
#error "Cannot detect target architecture. " \
       "If you are compiling for x86-64, " \
       "please remove and ignore this error."
#endif

static inline uint64_t rotl64(uint64_t x, uint64_t r) {
    return (x << r) | (x >> (64 - r));
}

static inline uint64_t fmix64(uint64_t k) {
    k ^= k >> 33;
    k *= 0xff51afd7ed558ccdull;
    k ^= k >> 33;
    k *= 0xc4ceb9fe1a85ec53ull;
    k ^= k >> 33;
    return k;
}

static uint64_t murmur_hash(const uint8_t *data, const size_t len, const uint32_t seed) {
    // MurmurHash3
    // https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp
    const size_t nblocks = len / 16;
    uint64_t h1 = seed, h2 = seed;
    const uint64_t c1 = 0x87c37b91114253d5ull,
                   c2 = 0x4cf5ad432745937full;
    const uint64_t *blocks = (const uint64_t *)data;
    for (size_t i = 0; i < nblocks; i++) {
        uint64_t k1 = blocks[i * 2    ],
                 k2 = blocks[i * 2 + 1];
        k1 *= c1; k1 = rotl64(k1, 31); k1 *= c2; h1 ^= k1;
        h1 = rotl64(h1, 27); h1 += h2; h1 = h1 * 5 + 0x52dce729;
        k2 *= c2; k2 = rotl64(k2, 33); k2 *= c1; h2 ^= k2;
        h2 = rotl64(h2, 31); h2 += h1; h2 = h2 * 5 + 0x38495ab5;
    }
    const uint8_t *tail = (const uint8_t *)(data + nblocks * 16);
    uint64_t k1 = 0, k2 = 0;
    switch (len & 15) {
    case 15: k2 ^= ((uint64_t)tail[14]) << 48; // fallthrough
    case 14: k2 ^= ((uint64_t)tail[13]) << 40; // fallthrough
    case 13: k2 ^= ((uint64_t)tail[12]) << 32; // fallthrough
    case 12: k2 ^= ((uint64_t)tail[11]) << 24; // fallthrough
    case 11: k2 ^= ((uint64_t)tail[10]) << 16; // fallthrough
    case 10: k2 ^= ((uint64_t)tail[ 9]) << 8; // fallthrough
    case  9: k2 ^= ((uint64_t)tail[ 8]) << 0;
             k2 *= c2; k2 = rotl64(k2, 33); k2 *= c1; h2 ^= k2;
             // fallthrough
    case  8: k1 ^= ((uint64_t)tail[ 7]) << 56; // fallthrough
    case  7: k1 ^= ((uint64_t)tail[ 6]) << 48; // fallthrough
    case  6: k1 ^= ((uint64_t)tail[ 5]) << 40; // fallthrough
    case  5: k1 ^= ((uint64_t)tail[ 4]) << 32; // fallthrough
    case  4: k1 ^= ((uint64_t)tail[ 3]) << 24; // fallthrough
    case  3: k1 ^= ((uint64_t)tail[ 2]) << 16; // fallthrough
    case  2: k1 ^= ((uint64_t)tail[ 1]) << 8; // fallthrough
    case  1: k1 ^= ((uint64_t)tail[ 0]) << 0;
             k1 *= c1; k1 = rotl64(k1, 31); k1 *= c2; h1 ^= k1;
    }
    h1 ^= len;
    h2 ^= len;
    h1 += h2;
    h2 += h1;
    h1 = fmix64(h1);
    h2 = fmix64(h2);
    h1 += h2;
    h2 += h1;
    return h1;
}

static inline uint64_t str_hash(const char *key, const size_t len) {
    return murmur_hash((const uint8_t *)key, len, HASHTABLE_HASH_SEED);
}
