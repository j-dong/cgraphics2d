#include "collision.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>

// aabb functions

float sweep_aabb(const AABB *box,
                 float x, float y, float idx, float idy,
                 int width, int height,
                 AABBEdge *edge) {
    // slab method for ray-aabb intersection
    float tx1 = (box->x1 - x - width) * idx;
    float tx2 = (box->x2 - x) * idx;
    bool xis1 = tx1 < tx2;
    float tmin = fminf(tx1, tx2);
    float tmax = fmaxf(tx1, tx2);
    float ty1 = (box->y1 - y - height) * idy;
    float ty2 = (box->y2 - y) * idy;
    bool yis1 = ty1 < ty2;
    float ntmin = fminf(ty1, ty2);
    bool isy = ntmin >= tmin;
    tmin = fmaxf(tmin, ntmin);
    tmax = fminf(tmax, fmaxf(ty1, ty2));
    if (tmin < tmax && tmin >= 0.0f && tmin < 1.0f) {
        if (edge)
            *edge = isy ? yis1 ? EDGE_NORTH : EDGE_SOUTH
                        : xis1 ? EDGE_WEST  : EDGE_EAST;
        return tmin;
    } else {
        return -1.0f;
    }
}

#ifndef __cplusplus
// inline aabb functions
void aabb_init(AABB *ret, int x1, int y1, int x2, int y2);
bool aabb_intersect(const AABB *a, const AABB *b);
bool aabb_contains(const AABB *a, const AABB *b);
void aabb_expand(AABB *a, int dx, int dy);
#endif

// quadtree functions

void quadtree_init(Quadtree *q, const AABB *box, size_t depth) {
    int x[] = {box->x1, (box->x1 + box->x2) / 2, box->x2};
    int y[] = {box->y1, (box->y1 + box->y2) / 2, box->y2};
    for (int i = 0; i < 4; i++) {
        aabb_init(&q->box[i], x[i & 1], y[i >> 1], x[(i & 1) + 1], y[(i >> 1) + 1]);
        q->child[i] = NULL;
    }
    q->max_depth = depth;
    q->data_len = 0;
    q->data_cap = 0;
    q->data = NULL;
}

void quadtree_delete(Quadtree *q) {
    if (q == NULL) return;
    free(q->data);
    for (int i = 0; i < 4; i++) quadtree_delete(q->child[i]);
    free(q);
}

static inline bool quadtree_should_subdivide(Quadtree *q) {
    return q->data_len >= QUADTREE_THRESHOLD && q->max_depth > 0;
}

// doesn't ever subdivide
static void quadtree_insert_trivial(Quadtree *q, void *el, size_t el_size) {
    if (q->data_len == q->data_cap) {
        q->data = realloc(q->data, q->data_cap * 2);
    }
    memcpy(q->data + q->data_len * el_size, el, el_size);
    q->data_len++;
}

static void quadtree_subdivide(Quadtree *q, size_t el_size) {
    for (int i = 0; i < 4; i++) {
        Quadtree *c = q->child[i] = malloc(sizeof(Quadtree));
        quadtree_init(c, &q->box[i], q->max_depth - 1);
        size_t in = 0, end = q->data_len, out = 0;
        while (in < end) {
            if (aabb_contains(&q->box[i], (AABB *)(q->data + in * el_size))) {
                quadtree_insert_trivial(c, q->data + in * el_size, el_size);
            } else {
                if (in != out) {
                    memcpy(q->data + out * el_size, q->data + in * el_size, el_size);
                }
                out++;
            }
            in++;
        }
        q->data_len = out;
        // subdivide if necessary
        if (quadtree_should_subdivide(c))
            quadtree_subdivide(c, el_size);
    }
}

static void quadtree_insert_leaf(Quadtree *q, void *el, size_t el_size) {
    quadtree_insert_trivial(q, el, el_size);
    if (quadtree_should_subdivide(q))
        quadtree_subdivide(q, el_size);
}

// returns true if it was contained in one of box
static bool quadtree_insert_children(Quadtree *q, void *el, size_t el_size) {
    for (int i = 0; i < 4; i++) {
        if (aabb_contains(&q->box[i], (AABB *)el)) {
            quadtree_insert(q->child[i], el, el_size);
            return true;
        }
    }
    return false;
}

void quadtree_insert(Quadtree *q, void *el, size_t el_size) {
    if (q->child[0] == NULL) {
        // unsubdivided
        quadtree_insert_leaf(q, el, el_size);
    } else {
        if (!quadtree_insert_children(q, el, el_size)) {
            quadtree_insert_leaf(q, el, el_size);
        }
    }
}

// remove element from leaf node or internal node where it doesn't fit into children
static void quadtree_move_impl_trivial(Quadtree *q, void *el, size_t el_size, qt_equal_fn equal, void *buf) {
    size_t i = 0;
    bool found = false;
    for (; i < q->data_len; i++) {
        if (equal(el, q->data + i * el_size)) {
            memcpy(buf, q->data + i * el_size, el_size);
            q->data_len--;
            found = true;
            break;
        }
    }
    if (!found) assert(!"element was not found in quadtree");
    for (; i < q->data_len; i++) {
        memcpy(q->data + i * el_size, q->data + (i + 1) * el_size, el_size);
    }
}

// returns true if element was inserted into its new position
static bool quadtree_move_impl(Quadtree *q, void *el, size_t el_size, qt_equal_fn equal, AABB *new_bounds, void *buf) {
    // walk through tree to find where it should be inserted
    // pretty similar to insert
    AABB *box = (AABB *)el;
    if (q->child[0] == NULL) {
        // unsubdivided
        quadtree_move_impl_trivial(q, el, el_size, equal, buf);
        memcpy(buf, new_bounds, sizeof(AABB));
        // since we don't have any children, we can't recurse
        return false;
    } else {
        bool found = false;
        for (int i = 0; i < 4; i++) {
            if (aabb_contains(&q->box[i], box)) {
                found = true;
                if (quadtree_move_impl(q->child[i], el, el_size, equal, new_bounds, buf))
                    return true;
            }
        }
        if (!found) {
            // it's stored in here as a "leaf"
            quadtree_move_impl_trivial(q, el, el_size, equal, buf);
            memcpy(buf, new_bounds, sizeof(AABB));
        }
        // by now, we've already moved the element into buf and modified its AABB
        // so just try to insert, but don't add it as a leaf node because it might
        // not fit into here
        return quadtree_insert_children(q, el, el_size);
    }
}

// NOTE: *guaranteed* that this is equivalent to removing then inserting again
// NOTE: *el will be modified; new_bounds will be copied to the start of el
void quadtree_move(Quadtree *q, void *el, size_t el_size, qt_equal_fn equal, AABB *new_bounds) {
    // temporary buffer for moved object
    void *buf = malloc(el_size);
    if (!quadtree_move_impl(q, el, el_size, equal, new_bounds, buf)) {
        // insert into root
        quadtree_insert_leaf(q, el, el_size);
    }
    free(buf);
}
