#include "hashtable.h"
#include <stdlib.h>

// hash table implementation

#define HASHTABLE_INITIAL_CAPACITY 8
#define HASHTABLE_LOAD_FACTOR_PERCENT 80

void hashtable_init(Hashtable *h) {
    h->data_len = 0;
    h->data_cap = HASHTABLE_INITIAL_CAPACITY;
    h->resize_cap = h->data_cap * HASHTABLE_LOAD_FACTOR_PERCENT / 100;
    h->mask = h->data_cap - 1;
    h->data = malloc(h->data_cap * sizeof *h->data);
}

void hashtable_free(Hashtable *h) {
}

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

static uint64_t str_hash(const uint8_t *key, const size_t len, const uint32_t seed) {
    // MurmurHash3
    // https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp
    const size_t nblocks = len / 16;
    uint64_t h1 = seed, h2 = seed;
    const uint64_t c1 = 0x87c37b91114253d5ull,
                   c2 = 0x4cf5ad432745937full;
    const uint64_t *blocks = (const uint64_t *)data;
    for (int i = 0; i < nblocks; i++) {
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
    case 15: k2 ^= ((uint64_t)tail[14]) << 48;
    case 14: k2 ^= ((uint64_t)tail[13]) << 40;
    case 13: k2 ^= ((uint64_t)tail[12]) << 32;
    case 12: k2 ^= ((uint64_t)tail[11]) << 24;
    case 11: k2 ^= ((uint64_t)tail[10]) << 16;
    case 10: k2 ^= ((uint64_t)tail[ 9]) << 8;
    case  9: k2 ^= ((uint64_t)tail[ 8]) << 0;
             k2 *= c2; k2 = rotl64(k2, 33); k2 *= c1; h2 ^= k2;
    case  8: k1 ^= ((uint64_t)tail[ 7]) << 56;
    case  7: k1 ^= ((uint64_t)tail[ 6]) << 48;
    case  6: k1 ^= ((uint64_t)tail[ 5]) << 40;
    case  5: k1 ^= ((uint64_t)tail[ 4]) << 32;
    case  4: k1 ^= ((uint64_t)tail[ 3]) << 24;
    case  3: k1 ^= ((uint64_t)tail[ 2]) << 16;
    case  2: k1 ^= ((uint64_t)tail[ 1]) << 8;
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
