#include "collision.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>

// aabb functions

double aabb_sweep(const AABB *box,
                  double x, double y, double idx, double idy,
                  int width, int height,
                  AABBEdge *edge) {
    // slab method for ray-aabb intersection
    double tx1 = (box->x1 - x - width) * idx;
    double tx2 = (box->x2 - x) * idx;
    bool xis1 = tx1 < tx2;
    double tmin = fmin(tx1, tx2);
    double tmax = fmax(tx1, tx2);
    double ty1 = (box->y1 - y - height) * idy;
    double ty2 = (box->y2 - y) * idy;
    bool yis1 = ty1 < ty2;
    double ntmin = fmin(ty1, ty2);
    bool isy = ntmin >= tmin;
    tmin = fmax(tmin, ntmin);
    tmax = fmin(tmax, fmax(ty1, ty2));
    if (tmin < tmax && tmin >= 0.0f && tmin < 1.0f) {
        if (edge)
            *edge = isy ? yis1 ? EDGE_NORTH : EDGE_SOUTH
                        : xis1 ? EDGE_WEST  : EDGE_EAST;
        return tmin;
    } else {
        return -1.0f;
    }
}

void aabb_init_bounding(AABB *a, double x, double y, double dx, double dy, int width, int height) {
    double x1 = x, x2 = x + width;
    double y1 = y, y2 = y + height;
    if (dx < 0) x1 += dx;
    else        x2 += dx;
    if (dy < 0) y1 += dy;
    else        y2 += dy;
    a->x1 = (int)floor(x1);
    a->x2 = (int)ceil(x2);
    a->y1 = (int)floor(y1);
    a->y2 = (int)ceil(y2);
}

#ifndef __cplusplus
// inline aabb functions
void aabb_init(AABB *ret, int x1, int y1, int x2, int y2);
bool aabb_intersect(const AABB *a, const AABB *b);
bool aabb_contains(const AABB *a, const AABB *b);
#endif

// quadtree impl

static const unsigned int QUADTREE_THRESHOLD = 16;

// quadtree data functions

static const char FREE_AABB[sizeof(AABB)];

static inline void quadtree_data_init(Quadtree *q) {
    q->data_len = 0;
    q->data_cap = 0;
    q->data_free = 0;
    q->data = NULL;
}

static inline void quadtree_data_delete(Quadtree *q) {
    free(q->data);
}

static void quadtree_data_remove_free(Quadtree *q) {
    if (q->data_free) {
        // similar to split_into_child
        size_t in = 0, end = q->data_len + q->data_free, out = 0;
        while (in < end) {
            if (memcmp((char *)q->data + in * q->el_size, FREE_AABB, sizeof(AABB)) != 0) {
                if (in != out)
                    memcpy((char *)q->data + out * q->el_size, (char *)q->data + in * q->el_size, q->el_size);
                out++;
            }
#ifndef NDEBUG
            else q->data_free--;
#endif
            in++;
        }
        assert(q->data_free == 0);
        q->data_free = 0;
    }
}

static void quadtree_data_insert(Quadtree *q, void *el) {
    if (q->data_cap == 0) {
        assert(q->data_len == 0 && q->data_free == 0);
        q->data_cap = 1;
        q->data = malloc(q->data_cap * q->el_size);
    }
    if (q->data_len + q->data_free >= q->data_cap) {
        quadtree_data_remove_free(q);
    }
    if (q->data_len == q->data_cap) {
        assert(q->data_free == 0);
        q->data_cap *= 2;
        q->data = realloc(q->data, q->data_cap * q->el_size);
    }
    memcpy((char *)q->data + (q->data_len + q->data_free) * q->el_size, el, q->el_size);
    q->data_len++;
}

// move elements from us to children satisfying predicate
// i is index of child
static void quadtree_data_split_into_child(Quadtree *q, int i) {
    size_t in = 0, end = q->data_len + q->data_free, out = 0;
    // remove freed elements while we're at it
    while (in < end) {
        void *inptr = (void *)((char *)q->data + in * q->el_size);
        if (memcmp(inptr, FREE_AABB, sizeof(AABB)) == 0) {
            // do nothing
#ifndef NDEBUG
            q->data_free--;
#endif
        } if (aabb_contains(&q->box[i], (AABB *)inptr)) {
            quadtree_data_insert(q->child[i], inptr);
        } else {
            if (in != out) {
                memcpy((char *)q->data + out * q->el_size, inptr, q->el_size);
            }
            out++;
        }
        in++;
    }
    q->data_len = out;
    assert(q->data_free == 0);
    q->data_free = 0;
}

static void quadtree_data_cleanup_after_split(Quadtree *q) {
    // shrink data if necessary
    assert(q->data_free == 0);
    if (q->data_cap / 2 >= q->data_len) {
        q->data_cap /= 2;
        while (q->data_cap && q->data_cap / 2 >= q->data_len)
            q->data_cap /= 2;
        if (q->data_cap == 0) {
            free(q->data);
            q->data = NULL;
        } else {
            q->data = realloc(q->data, q->data_cap * q->el_size);
        }
    }
}

static void quadtree_data_reserve(Quadtree *q, size_t new_len) {
    if (q->data_cap == 0) q->data_cap = 1;
    // if removing element from node with all zero-element children
    // while number of elements is one more than a power of two
    if (q->data_cap / 2 >= new_len) q->data_cap /= 2;
    // not really necessary, but I like having powers of two
    while (q->data_cap < new_len) {
        q->data_cap *= 2;
    }
    // possibility that we just reduced data_cap to 0
    if (q->data_cap) {
        quadtree_data_remove_free(q);
        q->data = realloc(q->data, q->data_cap * q->el_size);
    } else {
        free(q->data);
        q->data = NULL;
        q->data_free = 0;
    }
}

static inline void quadtree_data_unsplit_from_child(Quadtree *q, int i) {
    assert(q->data_free == 0);
    quadtree_data_remove_free(q->child[i]);
    memcpy((char *)q->data + q->data_len * q->el_size, q->child[i]->data, q->child[i]->data_len * q->el_size);
    q->data_len += q->child[i]->data_len;
}

static void quadtree_data_traverse(Quadtree *q, AABB *box, qt_callback_fn callback, void *cb_data) {
    for (size_t i = 0; i < q->data_len + q->data_free; i++) {
        void *d = (void *)((char *)q->data + i * q->el_size);
        if (memcmp(d, FREE_AABB, sizeof(AABB)) == 0) continue;
        AABB *dbox = (AABB *)d;
        if (aabb_intersect(box, dbox))
            callback(cb_data, d);
    }
}

// remove el from data and copy into buf, if given
static void quadtree_data_remove(Quadtree *q, void *el, qt_equal_fn equal, void *buf) {
    bool found = false;
    for (size_t i = 0; i < q->data_len + q->data_free; i++) {
        if (equal(el, (char *)q->data + i * q->el_size)) {
            if (buf)
                memcpy(buf, (char *)q->data + i * q->el_size, q->el_size);
            // mark as free
            memcpy((char *)q->data + i * q->el_size, FREE_AABB, sizeof(AABB));
            q->data_len--;
            q->data_free++;
            found = true;
            break;
        }
    }
    if (!found) assert(!"element was not found in quadtree");
    // memmove((char *)q->data + i * q->el_size, (char *)q->data + (i + 1) * q->el_size, (q->data_len - i) * q->el_size);
    // check if we can reduce capacity
    if (q->data_cap / 2 >= q->data_len) {
        q->data_cap /= 2;
        if (q->data_cap) {
            quadtree_data_remove_free(q);
            q->data = realloc(q->data, q->data_cap * q->el_size);
        } else {
            free(q->data);
            q->data = NULL;
            q->data_free = 0;
        }
    }
}

// quadtree functions

void quadtree_init(Quadtree *q, const AABB *box, size_t depth, size_t el_size) {
    int x[] = {box->x1, (box->x1 + box->x2) / 2, box->x2};
    int y[] = {box->y1, (box->y1 + box->y2) / 2, box->y2};
    for (int i = 0; i < 4; i++) {
        aabb_init(&q->box[i], x[i & 1], y[i >> 1], x[(i & 1) + 1], y[(i >> 1) + 1]);
        q->child[i] = NULL;
    }
    q->max_depth = depth;
    q->el_size = el_size;
    quadtree_data_init(q);
}

void quadtree_delete(Quadtree *q) {
    if (q == NULL) return;
    quadtree_data_delete(q);
    for (int i = 0; i < 4; i++) {
        quadtree_delete(q->child[i]);
        free(q->child[i]);
    }
}

static inline bool quadtree_should_subdivide(Quadtree *q) {
    return q->data_len >= QUADTREE_THRESHOLD && q->max_depth > 0;
}

static void quadtree_subdivide(Quadtree *q) {
    assert(q->max_depth > 0);
    for (int i = 0; i < 4; i++) {
        assert(!q->child[i] && "subdividing when already subdivided");
        Quadtree *c = q->child[i] = malloc(sizeof(Quadtree));
        quadtree_init(c, &q->box[i], q->max_depth - 1, q->el_size);
        quadtree_data_split_into_child(q, i);
        // subdivide if necessary
        if (quadtree_should_subdivide(c))
            quadtree_subdivide(c);
    }
    quadtree_data_cleanup_after_split(q);
}

static void quadtree_insert_leaf(Quadtree *q, void *el) {
    quadtree_data_insert(q, el);
    if (!q->child[0] && quadtree_should_subdivide(q))
        quadtree_subdivide(q);
}

// returns true if it was contained in one of box
static bool quadtree_insert_children(Quadtree *q, void *el) {
    for (int i = 0; i < 4; i++) {
        if (aabb_contains(&q->box[i], (AABB *)el)) {
            quadtree_insert(q->child[i], el);
            return true;
        }
    }
    return false;
}

void quadtree_insert(Quadtree *q, void *el) {
    if (q->child[0] == NULL) {
        // unsubdivided
        quadtree_insert_leaf(q, el);
    } else {
        if (!quadtree_insert_children(q, el)) {
            quadtree_insert_leaf(q, el);
        }
    }
}

// all children must not have children
static void quadtree_unsubdivide(Quadtree *q) {
    for (int i = 0; i < 4; i++) assert(!q->child[i]->child[0]);
    size_t new_len = q->data_len;
    for (int i = 0; i < 4; i++) new_len += q->child[i]->data_len;
    quadtree_data_reserve(q, new_len);
    // copy elements from children and delete
    for (int i = 0; i < 4; i++) {
        if (q->child[i]->data_len) {
            quadtree_data_unsplit_from_child(q, i);
        }
        quadtree_delete(q->child[i]);
        free(q->child[i]);
        q->child[i] = NULL;
    }
}

// returns true if element was inserted into its new position
// OR if we're just removing AND current node has children, so
// no need to prune further
static bool quadtree_move_impl(Quadtree *q, void *el, qt_equal_fn equal, AABB *new_bounds, void *buf) {
    // walk through tree to find where it should be inserted
    // pretty similar to insert
    AABB *box = (AABB *)el;
    if (q->child[0] == NULL) {
        // unsubdivided
        quadtree_data_remove(q, el, equal, buf);
        if (new_bounds)
            memcpy(buf, new_bounds, sizeof(AABB));
        // since we don't have any children, we can't recurse
        // no children, so can't unsubdivide
        return false;
    } else {
        bool found = false;
        for (int i = 0; i < 4; i++) {
            if (aabb_contains(&q->box[i], box)) {
                found = true;
                if (quadtree_move_impl(q->child[i], el, equal, new_bounds, buf))
                    return true;
                break; // no point in exploring other children
            }
        }
        if (!found) {
            // it's stored in here as a "leaf"
            quadtree_data_remove(q, el, equal, buf);
            if (new_bounds)
                memcpy(buf, new_bounds, sizeof(AABB));
        }
        // by now, we've already moved the element into buf and modified its AABB
        // so just try to insert, but don't add it as a leaf node because it might
        // not fit into here
        // return true if not unsubdividing because impossible to unsubdivide when
        // children have children
        bool ret = new_bounds ? quadtree_insert_children(q, buf) : true;
        // prune if few children
        // it's *probably* OK to prune after reinserting.
        size_t len = q->data_len;
        bool children_have_children = false;
        // if children have children, they must be over threshold
        // otherwise it'd be caught by recursive call already
        for (int i = 0; i < 4; i++) {
            if (q->child[i]->child[0]) {
                children_have_children = true;
                break;
            }
            len += q->child[i]->data_len;
        }
        if (len < QUADTREE_THRESHOLD && !children_have_children) {
            quadtree_unsubdivide(q);
            // we no longer have children, so caller should
            // check if it should also unsubdivide
            return false;
        }
        return ret;
    }
}

// NOTE: *guaranteed* that this is equivalent to removing then inserting again
// NOTE: moved element will be modified; new_bounds will be copied to the start
void quadtree_move(Quadtree *q, void *el, qt_equal_fn equal, AABB *new_bounds, void *buf) {
    if (!quadtree_move_impl(q, el, equal, new_bounds, buf)) {
        // insert into root
        quadtree_insert_leaf(q, buf);
    }
}

void quadtree_remove(Quadtree *q, void *el, qt_equal_fn equal, void *buf) {
    quadtree_move_impl(q, el, equal, NULL, buf);
}

void quadtree_traverse(Quadtree *q, AABB *box, qt_callback_fn callback, void *cb_data) {
    if (!q) return;
    quadtree_data_traverse(q, box, callback, cb_data);
    // check children
    for (int i = 0; i < 4; i++) {
        if (aabb_intersect(box, &q->box[i]))
            quadtree_traverse(q->child[i], box, callback, cb_data);
    }
}
