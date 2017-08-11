#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include "collision.h"

#define WIDTH 1024
#define HEIGHT 1024
#ifndef NUM_BOXES
#define NUM_BOXES 65536
#endif
#ifndef RAND_SEED
#define RAND_SEED 0
#endif
#ifndef SHIFT_AMOUNT
#define SHIFT_AMOUNT 64
#endif

#ifdef NDEBUG
#error "NDEBUG is defined"
#endif

struct box_t {
    AABB aabb;
    unsigned int idx;
};

typedef struct box_t Box;

static void randomize(Box *boxes) {
    srand(rand());
    for (int i = 0; i < NUM_BOXES; i++) {
        int x1 = rand() % WIDTH, x2 = rand() % WIDTH,
            y1 = rand() % HEIGHT, y2 = rand() % HEIGHT;
        if (x1 > x2) {
            int temp = x1;
            x1 = x2;
            x2 = temp;
        }
        if (y1 > y2) {
            int temp = y1;
            y1 = y2;
            y2 = temp;
        }
        aabb_init(&boxes[i].aabb, x1, y1, x2, y2);
    }
}

static void shift_random(Box *boxes) {
    srand(rand());
    for (int i = 0; i < NUM_BOXES; i++) {
        int sx = rand() % (SHIFT_AMOUNT * 2) - SHIFT_AMOUNT,
            sy = rand() % (SHIFT_AMOUNT * 2) - SHIFT_AMOUNT;
        boxes[i].aabb.x1 += sx;
        boxes[i].aabb.x2 += sx;
        boxes[i].aabb.y1 += sy;
        boxes[i].aabb.y2 += sy;
    }
}

static bool box_equal(void *a, void *b) {
    Box *aa = (Box *)a, *bb = (Box *)b;
    return aa->idx == bb->idx;
}

static void print_idx(void *_, void *a) {
    // do nothing
    _, a;
}

static bool aabb_equal(AABB *a, AABB *b) {
    return a->x1 == b->x1
        && a->y1 == b->y1
        && a->x2 == b->x2
        && a->y2 == b->y2;
}

static void quadtree_assert_equiv(Quadtree *a, Quadtree *b) {
    if (a == NULL && b == NULL)
        return;
    if (a == b)
        assert(!"checking equivalence of equal pointers");
    for (int i = 0; i < 4; i++)
        assert(aabb_equal(&a->box[i], &b->box[i]));
    assert(a->max_depth == b->max_depth);
    assert(a->data_len == b->data_len);
    // technically shouldn't be checked, but
    // I want to ensure that we're not wasting memory
    assert(a->data_cap == b->data_cap);
    // make sure they contain the same elements
    static Box *temp[NUM_BOXES] = {0};
    Box *a_data = (Box *)a->data,
        *b_data = (Box *)b->data;
    for (size_t i = 0; i < a->data_len; i++) {
        temp[a_data[i].idx] = (Box *)&a_data[i];
    }
    for (size_t i = 0; i < b->data_len; i++) {
        assert(temp[b_data[i].idx] != NULL);
        assert(aabb_equal(&temp[b_data[i].idx]->aabb, &b_data[i].aabb));
        temp[b_data[i].idx] = NULL;
    }
    for (size_t i = 0; i < NUM_BOXES; i++)
        assert(temp[i] == NULL);
    // check children
    for (int i = 0; i < 4; i++)
        quadtree_assert_equiv(a->child[i], b->child[i]);
}

int main() {
    srand(RAND_SEED);
    AABB bounds;
    aabb_init(&bounds, 0, 0, WIDTH, HEIGHT);
    Quadtree q, q_new;
    quadtree_init(&q, &bounds, 8);
    quadtree_init(&q_new, &bounds, 8);
    Box *boxes = malloc(sizeof(Box) * NUM_BOXES),
        *new_pos = malloc(sizeof(Box) * NUM_BOXES);
    for (int i = 0; i < NUM_BOXES; i++) {
        boxes[i].idx = i;
        new_pos[i].idx = i;
    }
    randomize(boxes);
    memcpy(new_pos, boxes, NUM_BOXES * sizeof(Box));
    shift_random(new_pos);
    clock_t start, end;
    // begin time insert
    start = clock();
    for (int i = 0; i < NUM_BOXES; i++)
        quadtree_insert(&q, &boxes[i], sizeof(Box));
    end = clock();
    fprintf(stderr, "inserting %d elements took %.3f ms.\n", NUM_BOXES, (end - start) * 1000.0 / CLOCKS_PER_SEC);
    // end time insert
    // make sure everything's in there
    quadtree_traverse(&q, &bounds, sizeof(Box), print_idx, NULL);
    // move everything around and make sure it's the same structure as inserting
    // begin time move
    start = clock();
    for (int i = 0; i < NUM_BOXES; i++) {
        quadtree_move(&q, &boxes[i], sizeof(Box), box_equal, &new_pos[i].aabb);
        // below works
        // quadtree_remove(&q, &boxes[i], sizeof(Box), box_equal, NULL);
        // boxes[i].aabb = new_pos[i].aabb;
        // quadtree_insert(&q, &boxes[i], sizeof(Box));
    }
    end = clock();
    fprintf(stderr, "moving %d elements took %.3f ms.\n", NUM_BOXES, (end - start) * 1000.0 / CLOCKS_PER_SEC);
    // end time move
    // update box positions
    for (int i = 0; i < NUM_BOXES; i++)
        boxes[i].aabb = new_pos[i].aabb;
    // begin time insert
    start = clock();
    for (int i = 0; i < NUM_BOXES; i++)
        quadtree_insert(&q_new, &boxes[i], sizeof(Box));
    end = clock();
    fprintf(stderr, "inserting %d elements took %.3f ms.\n", NUM_BOXES, (end - start) * 1000.0 / CLOCKS_PER_SEC);
    // end time insert
    quadtree_assert_equiv(&q, &q_new);
    // begin time remove
    start = clock();
    Box temp;
    for (size_t i = 0; i < NUM_BOXES; i++) {
        quadtree_remove(&q, &boxes[i], sizeof(Box), box_equal, &temp);
        assert(temp.idx == boxes[i].idx && boxes[i].idx == i);
        assert(aabb_equal(&temp.aabb, &boxes[i].aabb));
        assert(aabb_equal(&temp.aabb, &new_pos[i].aabb));
    }
    end = clock();
    fprintf(stderr, "removing %d elements took %.3f ms.\n", NUM_BOXES, (end - start) * 1000.0 / CLOCKS_PER_SEC);
    assert(!q.child[0] && "root is subdivided after removing all children");
    assert(q.data_len == 0 && "root has children after removing children");
    // end time remove
    quadtree_delete(&q);
    quadtree_delete(&q_new);
    free(boxes);
    free(new_pos);
    return 0;
}
