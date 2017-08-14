#ifndef INCLUDED_GRAPHICS_COLLISION_H
#define INCLUDED_GRAPHICS_COLLISION_H

#include <stddef.h>
#include <stdbool.h>

// collision detection structures and functions
// TODO: make sure it actually works

// AABBs (Axis-Aligned Bounding Box)

struct aabb_t {
    int x1, y1, x2, y2;
};
// x2 and y2 are not included

typedef struct aabb_t AABB;

enum aabb_edge_t {
    EDGE_WEST = 0,
    EDGE_EAST = 1,
    EDGE_NORTH = 2,
    EDGE_SOUTH = 3,
};

// some tricks:
// let e be an edge.
// e >> 1 is
//  0 if x (west or east) and
//  1 if y (north or south)
// e & 1 is
//  0 if negative edge (west or north) and
//  1 if positive edge (east or south)

typedef enum aabb_edge_t AABBEdge;

inline void aabb_init(AABB *ret, int x1, int y1, int x2, int y2) {
    ret->x1 = x1;
    ret->y1 = y1;
    ret->x2 = x2;
    ret->y2 = y2;
}

inline bool aabb_intersect(const AABB *a, const AABB *b) {
    return !(b->x1 >= a->x2
          || b->x2 <= a->x1
          || b->y1 >= a->y2
          || b->y2 <= a->y1);
}

// if a contains b (edges touching are OK)
inline bool aabb_contains(const AABB *a, const AABB *b) {
    return b->x1 >= a->x1
        && b->x2 <= a->x2
        && b->y1 >= a->y1
        && b->y2 <= a->y2;
}

// Finds the bounding AABB for a swept AABB.
void aabb_init_bounding(AABB *a, double x, double y, double dx, double dy, int width, int height);

// Sweep an AABB to another.
// Subtracts rectangle (width, height) -> ray-AABB intersection
// with expanded AABB (x1 and y1 are decreased by width and height).
// returns t (the fraction of displacement before ray hits AABB)
// sets edge to the edge intersected
// idx and idy are 1.0 / displacement
// returns -1.0 if no collision
double aabb_sweep(const AABB *box,
                  double x, double y, double idx, double idy,
                  int width, int height,
                  AABBEdge *edge);

// Quadtrees

enum quadtree_index_t {
    NORTHWEST = 0,
    NORTHEAST = 1,
    SOUTHWEST = 2,
    SOUTHEAST = 3,
};

typedef enum quadtree_index_t QuadtreeIndex;

struct quadtree_t {
    AABB box[4];
    struct quadtree_t *child[4];
    size_t max_depth;
    size_t data_len;
    size_t data_cap;
    size_t data_free;
    size_t el_size;
    void *data; // data_len * arbitrary-sized elements
    // element size is given as parameter to methods
    // elements should be POD (no destructor and memcpy-able)
    // element type should be a struct where AABB is the first member
    // freed elements are represented by a zeroed-out AABB
};

typedef struct quadtree_t Quadtree;
typedef bool (*qt_equal_fn)(void *a, void *b);
typedef void (*qt_callback_fn)(void *data, void *a);

void quadtree_init(Quadtree *q, const AABB *box, size_t depth, size_t el_size);
// Frees quadtree and children.
// Does not free parameter.
void quadtree_free(Quadtree *q);
// Insert into a quadtree. Do not insert elements of different sizes.
void quadtree_insert(Quadtree *q, void *el);
// Move object inside quadtree. Compares data using given comparator.
// equal is a function pointer that returns something nonzero if
// the two parameters are equal.
// buf is a buffer of size el_size. It will contain the moved element.
// Moved object will have new_bounds as initial element of struct.
void quadtree_move(Quadtree *q, void *el, qt_equal_fn equal, AABB *new_bounds, void *buf);
// Remove object from quadtree. Copies to buf if non-NULL.
void quadtree_remove(Quadtree *q, void *el, qt_equal_fn equal, void *buf);
// Traverse quadtree, calling callback for each intersection.
// cb_data will be provided as first argument to callback.
// The element data is provided as second argument.
void quadtree_traverse(Quadtree *q, AABB *box, qt_callback_fn callback, void *cb_data);
// Clone quadtree.
void quadtree_clone(Quadtree *dest, const Quadtree *src);

#endif
