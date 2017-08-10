#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include "collision.h"

#define WIDTH 1024
#define HEIGHT 1024
#define NUM_BOXES 21

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

static bool box_equal(void *a, void *b) {
    Box *aa = (Box *)a, *bb = (Box *)b;
    printf("comparing %d and %d\n", aa->idx, bb->idx);
    return aa->idx == bb->idx;
}

static void print_idx(void *_, void *a) {
    Box *aa = (Box *)a;
    printf("%05d\n", aa->idx);
}

int main() {
    srand(0);
    AABB bounds;
    aabb_init(&bounds, 0, 0, WIDTH, HEIGHT);
    Quadtree q;
    quadtree_init(&q, &bounds, 8);
    Box boxes[NUM_BOXES]; //, new_pos[NUM_BOXES];
    for (int i = 0; i < NUM_BOXES; i++)
        boxes[i].idx = i;
    randomize(boxes);
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
    // begin time remove
    start = clock();
    for (int i = 0; i < NUM_BOXES; i++)
        quadtree_remove(&q, &boxes[i], sizeof(Box), box_equal, NULL);
    end = clock();
    fprintf(stderr, "removing %d elements took %.3f ms.\n", NUM_BOXES, (end - start) * 1000.0 / CLOCKS_PER_SEC);
    // end time remove
    quadtree_delete(&q);
    return 0;
}
