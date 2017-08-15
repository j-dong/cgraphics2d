#include "hashtable.h"
#include <stdio.h>
#include <assert.h>
#include <time.h>

#ifdef NDEBUG
#define TEST_ASSERT(x) do { if (!(x)) { fprintf(stderr, "Assertion failed: (%s:%d) %s\n", __FILE__, __LINE__, #x); abort(); } } while (false)
#else
#define TEST_ASSERT assert
#endif

#ifndef DO_INTENSIVE_BENCHMARK
#define DO_INTENSIVE_BENCHMARK 1
#endif

#ifdef __WIN32__
#define PF_SIZE_T "%Iu"
#else
#define PF_SIZE_T "%zu"
#endif

#define NUM_STRINGS 65536

const char *some_strings[] = {
    "hello",
    "world",
    "apple",
    "orange",
    "watermelon",
    "foo",
    "bar",
    "baz",
    "spam",
    "ham",
    "eggs",
    "test",
    "alice",
    "bob",
    "eve",
    "adam",
};

char strings[NUM_STRINGS][5];
const void *values[NUM_STRINGS];

void print_key_value(const char *key, void *value) {
    printf("%s => %s\n", key, (char *)value);
}

void verify_lookup(void *h, const char *key, void *value) {
    void *lookup_value = hashtable_get((Hashtable *)h, key);
    TEST_ASSERT(lookup_value == value);
}

void verify_values(const char *key, void *value) {
    size_t i = (key[0] - 'a')
             | (key[1] - 'a') << 4
             | (key[2] - 'a') << 8
             | (key[3] - 'a') << 12;
    TEST_ASSERT(value == values[i]);
}

int main() {
    Hashtable hash, *h = &hash;
    printf("=== initializing hashtable ===\n");
    hashtable_init(h);
    printf("=== printing empty hashtable ===\n");
    hashtable_traverse(h, print_key_value);
    printf("\n");
    printf("=== filling hashtable halfway ===\n");
    for (int i = 0; i < 8; i += 2)
        hashtable_put(h, some_strings[i], (void *)some_strings[i + 1]);
    printf("=== printing half-full hashtable (capacity: " PF_SIZE_T ") ===\n", h->data_cap);
    hashtable_traverse(h, print_key_value);
    printf("\n");
    printf("=== verifying lookup ===\n");
    hashtable_traverse_data(h, h, verify_lookup);
    printf("\n");
    printf("=== overwriting some elements ===\n");
    for (int i = 0; i < 4; i += 2)
        hashtable_put(h, some_strings[i], (void *)some_strings[i + 3]);
    printf("=== printing new hashtable ===\n");
    hashtable_traverse(h, print_key_value);
    printf("\n");
    printf("=== filling hashtable some more ===\n");
    for (int i = 8; i < 16; i += 2)
        hashtable_put(h, some_strings[i], (void *)some_strings[i + 1]);
    printf("=== printing full hashtable (capacity: " PF_SIZE_T ") ===\n", h->data_cap);
    hashtable_traverse(h, print_key_value);
    printf("\n");
    printf("=== removing some elements ===\n");
    for (int i = 0; i < 8; i += 2)
        hashtable_remove(h, some_strings[i]);
    printf("=== printing removed hashtable (capacity: " PF_SIZE_T ") ===\n", h->data_cap);
    hashtable_traverse(h, print_key_value);
    printf("\n");
    printf("=== adding elements back ===\n");
    for (int i = 0; i < 16; i += 2)
        hashtable_put(h, some_strings[i], (void *)some_strings[(i + 3) % 16]);
    printf("=== printing readded hashtable (capacity: " PF_SIZE_T ") ===\n", h->data_cap);
    hashtable_traverse(h, print_key_value);
    printf("\n");
    printf("=== freeing hashtable ===\n");
    hashtable_free(h);
#if DO_INTENSIVE_BENCHMARK
    printf("\n");
    printf("*** intensive benchmark ***\n");
    // set up strings
    for (size_t i = 0; i < NUM_STRINGS; i++) {
        strings[i][0] = ((i      ) & 0x000F) + 'a';
        strings[i][1] = ((i >>  4) & 0x000F) + 'a';
        strings[i][2] = ((i >>  8) & 0x000F) + 'a';
        strings[i][3] = ((i >> 12) & 0x000F) + 'a';
        strings[i][4] = 0;
    }
    clock_t start, end;
    printf("=== initializing hashtable ===\n");
    hashtable_init(h);
    printf("=== inserting %d elements ===\n", NUM_STRINGS);
    start = clock();
    for (size_t i = 0; i < NUM_STRINGS; i++) {
        hashtable_put(h, strings[i], (void *)some_strings[i % 16]);
        values[i] = some_strings[i % 16];
    }
    end = clock();
    printf("    took %.3f ms\n", (end - start) * 1000.0 / CLOCKS_PER_SEC);
    TEST_ASSERT(h->data_len == NUM_STRINGS);
    hashtable_traverse(h, verify_values);
    printf("=== overwriting %d elements ===\n", NUM_STRINGS);
    start = clock();
    for (size_t j = 0; j < NUM_STRINGS; j++) {
        size_t i = (-j + NUM_STRINGS / 1 + NUM_STRINGS) % NUM_STRINGS;
        hashtable_put(h, strings[i], (void *)some_strings[j % 16]);
        values[i] = some_strings[j % 16];
    }
    end = clock();
    printf("    took %.3f ms\n", (end - start) * 1000.0 / CLOCKS_PER_SEC);
    TEST_ASSERT(h->data_len == NUM_STRINGS);
    hashtable_traverse(h, verify_values);
    printf("=== removing %d elements ===\n", NUM_STRINGS / 2);
    start = clock();
    for (size_t i = 0; i < NUM_STRINGS / 2; i++) {
        TEST_ASSERT(hashtable_remove(h, strings[i]) == values[i]);
        values[i] = NULL;
    }
    end = clock();
    printf("    took %.3f ms\n", (end - start) * 1000.0 / CLOCKS_PER_SEC);
    TEST_ASSERT(h->data_len == NUM_STRINGS / 2);
    hashtable_traverse(h, verify_values);
    printf("=== readding %d elements ===\n", NUM_STRINGS);
    start = clock();
    for (size_t i = 0; i < NUM_STRINGS; i++) {
        hashtable_put(h, strings[i], (void *)some_strings[i % 16]);
        values[i] = some_strings[i % 16];
    }
    end = clock();
    printf("    took %.3f ms\n", (end - start) * 1000.0 / CLOCKS_PER_SEC);
    TEST_ASSERT(h->data_len == NUM_STRINGS);
    hashtable_traverse(h, verify_values);
    printf("=== freeing hashtable ===\n");
    hashtable_free(h);
    printf("\n");
#endif
}
