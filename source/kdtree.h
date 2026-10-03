// kdtree.h
//
// A spatial KD-tree built over a triangle mesh.
//
// WHY A KD-TREE?
// ------------------------------------------------------------
// Once an island is an arbitrary lumpy mesh (not a perfect dome), the only
// way to ask "am I touching it?" is to test against its triangles. Testing
// every triangle every frame is slow. A KD-tree fixes that by recursively
// chopping space in half:
//
//                      [ whole island ]
//                       split on X = 3.2
//                      /               |
//             [ x <= 3.2 ]          [ x >= 3.2 ]
//          split on Z = -1.0      split on Y = 0.4
//            /        |             /        |
//          ...       ...          ...       ...
//
// Each leaf holds only a handful of triangles. A query (ray, sphere, ...)
// starts at the root and only walks into the halves it could possibly
// touch, so most triangles are never looked at.
//
// A triangle that straddles a split plane is simply stored in BOTH halves.
//
// WHAT YOU CAN ASK IT
// ------------------------------------------------------------
//   kdtreeRaycast        - first triangle hit by a ray (ground height, camera)
//   kdtreeSphereOverlap  - is a sphere touching any triangle? (boat)
//   kdtreeSpherePush     - how far must a sphere move to get out? (player walls)
//
// The tree is built ONCE per island (islands never deform) and then only
// queried. Triangles are copied in, so the source arrays can be freed.

#pragma once
#include "common.h"
#include <stdbool.h>

// ---- Tuning ----
#define KD_LEAF_MAX_TRIS  4    // Stop splitting when a node has this few triangles
#define KD_MAX_DEPTH      16   // Hard cap on tree depth (guards against bad input)

// One triangle: three corners plus a precomputed unit normal.
typedef struct {
    Vec3 a, b, c;
    Vec3 n;          // Unit-length face normal (used to tell floors from walls)
} KDTriangle;

// One node of the tree. Stored in a flat array and linked by index (not
// pointers) so the array can be realloc'd while building.
typedef struct {
    int   axis;      // 0 = X, 1 = Y, 2 = Z split. -1 means this node is a leaf.
    float split;     // Position of the splitting plane along `axis`
    int   left;      // Child index: everything with coordinate <= split
    int   right;     // Child index: everything with coordinate >= split
    int   first;     // LEAF ONLY: where this leaf's triangle ids start in `indices`
    int   count;     // LEAF ONLY: how many triangle ids it has
} KDNode;

typedef struct {
    KDTriangle* tris;      // All triangles (owned by the tree)
    int         triCount;

    KDNode*     nodes;     // Flat node array; node 0 is the root
    int         nodeCount;
    int         nodeCap;

    int*        indices;   // Triangle ids referenced by the leaves
    int         indexCount;
    int         indexCap;

    Vec3        boundsMin; // Box around the entire mesh
    Vec3        boundsMax;
} KDTree;

// Result of a ray cast.
typedef struct {
    float t;         // Distance along the ray to the hit
    Vec3  point;     // World position of the hit
    Vec3  normal;    // Unit normal of the triangle that was hit
} KDHit;

// ---- Lifecycle ----

// Builds a tree from `triCount` triangles given as 3 corners each
// (verts[i*3], verts[i*3+1], verts[i*3+2]). Degenerate (zero-area)
// triangles are skipped. Returns false if out of memory or no usable triangles.
bool kdtreeBuild(KDTree* tree, const Vec3* verts, int triCount);

// Frees everything the tree owns and resets it to empty. Safe to call twice.
void kdtreeFree(KDTree* tree);

// ---- Queries ----

// Casts a ray from `origin` along `dir` (need not be normalised) out to
// `maxDist` (measured in units of `dir`'s length after normalising).
// Returns true and fills `hit` if any triangle is struck.
bool kdtreeRaycast(const KDTree* tree, Vec3 origin, Vec3 dir, float maxDist, KDHit* hit);

// True if a sphere at `center` with `radius` touches any triangle.
bool kdtreeSphereOverlap(const KDTree* tree, Vec3 center, float radius);

// Finds the deepest penetration of the sphere into the mesh and returns the
// vector that would push the sphere out of it. Only triangles whose normal
// satisfies |n.y| <= maxNormalY are considered, so passing e.g. 0.7 makes the
// sphere collide with steep WALLS but ignore gentle FLOORS (the player walks
// over floors using the ground ray instead). Pass 1.0 to collide with all.
bool kdtreeSpherePush(const KDTree* tree, Vec3 center, float radius,
                      float maxNormalY, Vec3* outPush);
