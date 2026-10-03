// kdtree.c
//
// KD-tree over triangles: build, ray cast, and sphere queries.
// See kdtree.h for the big-picture explanation of what a KD-tree is and why
// the game uses one. This file is the "how".
//
// File layout:
//   1. tiny vector helpers
//   2. building the tree
//   3. triangle math (ray-vs-triangle, closest-point-on-triangle)
//   4. ray queries
//   5. sphere queries

#include <stdlib.h>
#include <math.h>
#include "kdtree.h"

// ============================================================
// SECTION 1: tiny vector helpers
// ============================================================

static inline Vec3  vSub (Vec3 a, Vec3 b) { return (Vec3){ a.x-b.x, a.y-b.y, a.z-b.z }; }
static inline Vec3  vAdd (Vec3 a, Vec3 b) { return (Vec3){ a.x+b.x, a.y+b.y, a.z+b.z }; }
static inline Vec3  vMul (Vec3 a, float s){ return (Vec3){ a.x*s, a.y*s, a.z*s }; }
static inline float vDot (Vec3 a, Vec3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
static inline Vec3  vCross(Vec3 a, Vec3 b) {
    return (Vec3){ a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x };
}

// Pick x, y or z by number (0, 1, 2). Lets the same code split on any axis.
static inline float comp(Vec3 v, int axis) {
    return axis == 0 ? v.x : (axis == 1 ? v.y : v.z);
}

static inline float minf3(float a, float b, float c) { return fminf(a, fminf(b, c)); }
static inline float maxf3(float a, float b, float c) { return fmaxf(a, fmaxf(b, c)); }

// Lowest / highest value of a triangle along one axis.
static inline float triMin(const KDTriangle* t, int axis) {
    return minf3(comp(t->a, axis), comp(t->b, axis), comp(t->c, axis));
}
static inline float triMax(const KDTriangle* t, int axis) {
    return maxf3(comp(t->a, axis), comp(t->b, axis), comp(t->c, axis));
}

// ============================================================
// SECTION 2: building the tree
// ============================================================
//
// Recipe (applied recursively to a list of triangle ids):
//   1. Few triangles left (or too deep)?  -> make a LEAF holding them.
//   2. Otherwise find the box around them and pick its LONGEST axis.
//   3. Put the splitting plane at the MEDIAN triangle centre on that axis,
//      so both halves get roughly half the triangles (a balanced tree).
//   4. Triangles fully on one side go to that side; triangles touching the
//      plane go to both.
//   5. If that failed to separate anything, give up and make a leaf.
//   6. Recurse on each half.

// Reserve one more node and return its index (or -1 if out of memory).
static int pushNode(KDTree* t) {
    if (t->nodeCount >= t->nodeCap) {
        int newCap = t->nodeCap ? t->nodeCap * 2 : 64;
        KDNode* grown = (KDNode*)realloc(t->nodes, (size_t)newCap * sizeof(KDNode));
        if (!grown) return -1;
        t->nodes   = grown;
        t->nodeCap = newCap;
    }
    return t->nodeCount++;
}

// Turn node `self` into a leaf that owns the triangle ids in `list`.
static int makeLeaf(KDTree* t, int self, const int* list, int n) {
    if (t->indexCount + n > t->indexCap) {
        int newCap = t->indexCap ? t->indexCap * 2 : 256;
        while (newCap < t->indexCount + n) newCap *= 2;
        int* grown = (int*)realloc(t->indices, (size_t)newCap * sizeof(int));
        if (!grown) return -1;
        t->indices   = grown;
        t->indexCap  = newCap;
    }
    int first = t->indexCount;
    for (int i = 0; i < n; i++) t->indices[t->indexCount++] = list[i];

    t->nodes[self] = (KDNode){ -1, 0.0f, -1, -1, first, n };
    return self;
}

static int cmpFloat(const void* a, const void* b) {
    float fa = *(const float*)a, fb = *(const float*)b;
    return (fa > fb) - (fa < fb);
}

static int buildNode(KDTree* t, const int* list, int n, int depth) {
    int self = pushNode(t);
    if (self < 0) return -1;

    // Step 1: small enough (or deep enough) -> leaf.
    if (n <= KD_LEAF_MAX_TRIS || depth >= KD_MAX_DEPTH)
        return makeLeaf(t, self, list, n);

    // Step 2: box around this node's triangles, then its longest axis.
    float lo[3] = {  1e30f,  1e30f,  1e30f };
    float hi[3] = { -1e30f, -1e30f, -1e30f };
    for (int i = 0; i < n; i++) {
        const KDTriangle* tri = &t->tris[list[i]];
        for (int a = 0; a < 3; a++) {
            lo[a] = fminf(lo[a], triMin(tri, a));
            hi[a] = fmaxf(hi[a], triMax(tri, a));
        }
    }
    int axis = 0;
    if (hi[1]-lo[1] > hi[axis]-lo[axis]) axis = 1;
    if (hi[2]-lo[2] > hi[axis]-lo[axis]) axis = 2;

    // Step 3: splitting plane at the median triangle centre.
    float* centres = (float*)malloc((size_t)n * sizeof(float));
    int*   leftList  = (int*)malloc((size_t)n * sizeof(int));
    int*   rightList = (int*)malloc((size_t)n * sizeof(int));
    if (!centres || !leftList || !rightList) {
        free(centres); free(leftList); free(rightList);
        return -1;
    }
    for (int i = 0; i < n; i++) {
        const KDTriangle* tri = &t->tris[list[i]];
        centres[i] = (comp(tri->a, axis) + comp(tri->b, axis) + comp(tri->c, axis)) / 3.0f;
    }
    qsort(centres, (size_t)n, sizeof(float), cmpFloat);
    float split = centres[n / 2];
    free(centres);

    // Step 4: sort triangles into the two halves (straddlers go in both).
    int nl = 0, nr = 0;
    for (int i = 0; i < n; i++) {
        const KDTriangle* tri = &t->tris[list[i]];
        if (triMin(tri, axis) <= split) leftList [nl++] = list[i];
        if (triMax(tri, axis) >= split) rightList[nr++] = list[i];
    }

    // Step 5: no progress (everything straddles the plane) -> leaf.
    if (nl == n || nr == n) {
        free(leftList); free(rightList);
        return makeLeaf(t, self, list, n);
    }

    // Step 6: recurse. Note: we only keep INDICES to nodes, never pointers,
    // because pushNode() may realloc the node array during recursion.
    int left  = buildNode(t, leftList,  nl, depth + 1);
    int right = buildNode(t, rightList, nr, depth + 1);
    free(leftList); free(rightList);
    if (left < 0 || right < 0) return -1;

    t->nodes[self] = (KDNode){ axis, split, left, right, 0, 0 };
    return self;
}

bool kdtreeBuild(KDTree* t, const Vec3* verts, int triCount) {
    *t = (KDTree){0};
    if (triCount <= 0) return false;

    t->tris = (KDTriangle*)malloc((size_t)triCount * sizeof(KDTriangle));
    int*  all = (int*)malloc((size_t)triCount * sizeof(int));
    if (!t->tris || !all) { free(all); kdtreeFree(t); return false; }

    // Copy triangles in, computing each normal. Skip zero-area triangles
    // (the dome's peak ring collapses to a point and produces some).
    for (int i = 0; i < triCount; i++) {
        Vec3 a = verts[i*3], b = verts[i*3+1], c = verts[i*3+2];
        Vec3 cr = vCross(vSub(b, a), vSub(c, a));
        float len = sqrtf(vDot(cr, cr));
        if (len < 1e-7f) continue;

        t->tris[t->triCount] = (KDTriangle){ a, b, c, vMul(cr, 1.0f / len) };
        all[t->triCount] = t->triCount;
        t->triCount++;
    }
    if (t->triCount == 0) { free(all); kdtreeFree(t); return false; }

    // Box around the whole mesh -- lets ray queries reject misses instantly.
    t->boundsMin = (Vec3){  1e30f,  1e30f,  1e30f };
    t->boundsMax = (Vec3){ -1e30f, -1e30f, -1e30f };
    for (int i = 0; i < t->triCount; i++) {
        const KDTriangle* tri = &t->tris[i];
        t->boundsMin.x = fminf(t->boundsMin.x, triMin(tri, 0));
        t->boundsMin.y = fminf(t->boundsMin.y, triMin(tri, 1));
        t->boundsMin.z = fminf(t->boundsMin.z, triMin(tri, 2));
        t->boundsMax.x = fmaxf(t->boundsMax.x, triMax(tri, 0));
        t->boundsMax.y = fmaxf(t->boundsMax.y, triMax(tri, 1));
        t->boundsMax.z = fmaxf(t->boundsMax.z, triMax(tri, 2));
    }

    int root = buildNode(t, all, t->triCount, 0);
    free(all);
    if (root != 0) { kdtreeFree(t); return false; }   // root is always node 0
    return true;
}

void kdtreeFree(KDTree* t) {
    free(t->tris);
    free(t->nodes);
    free(t->indices);
    *t = (KDTree){0};
}

// ============================================================
// SECTION 3: triangle math
// ============================================================

// Moller-Trumbore ray/triangle test. Double-sided (hits front OR back),
// which matters for the camera ray when the camera is inside an island.
static bool rayTriangle(Vec3 o, Vec3 d, const KDTriangle* tri, float* outT) {
    Vec3 e1 = vSub(tri->b, tri->a);
    Vec3 e2 = vSub(tri->c, tri->a);
    Vec3 p  = vCross(d, e2);
    float det = vDot(e1, p);
    if (fabsf(det) < 1e-9f) return false;          // Ray parallel to triangle

    float inv = 1.0f / det;
    Vec3  s   = vSub(o, tri->a);
    float u   = vDot(s, p) * inv;                  // Barycentric coordinate u
    if (u < 0.0f || u > 1.0f) return false;

    Vec3  q = vCross(s, e1);
    float v = vDot(d, q) * inv;                    // Barycentric coordinate v
    if (v < 0.0f || u + v > 1.0f) return false;

    float t = vDot(e2, q) * inv;                   // Distance along the ray
    if (t < 0.0f) return false;
    *outT = t;
    return true;
}

// Closest point on triangle (a,b,c) to point p. Classic region-based
// method: figure out whether p's nearest spot is a corner, an edge, or the
// inside of the face. (Ericson, "Real-Time Collision Detection".)
static Vec3 closestPointOnTriangle(Vec3 p, Vec3 a, Vec3 b, Vec3 c) {
    Vec3 ab = vSub(b, a), ac = vSub(c, a), ap = vSub(p, a);
    float d1 = vDot(ab, ap), d2 = vDot(ac, ap);
    if (d1 <= 0.0f && d2 <= 0.0f) return a;                       // corner A

    Vec3 bp = vSub(p, b);
    float d3 = vDot(ab, bp), d4 = vDot(ac, bp);
    if (d3 >= 0.0f && d4 <= d3) return b;                         // corner B

    float vc = d1*d4 - d3*d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)                   // edge AB
        return vAdd(a, vMul(ab, d1 / (d1 - d3)));

    Vec3 cp = vSub(p, c);
    float d5 = vDot(ab, cp), d6 = vDot(ac, cp);
    if (d6 >= 0.0f && d5 <= d6) return c;                         // corner C

    float vb = d5*d2 - d1*d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)                   // edge AC
        return vAdd(a, vMul(ac, d2 / (d2 - d6)));

    float va = d3*d6 - d5*d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {   // edge BC
        float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        return vAdd(b, vMul(vSub(c, b), w));
    }

    float denom = 1.0f / (va + vb + vc);                          // inside face
    return vAdd(a, vAdd(vMul(ab, vb * denom), vMul(ac, vc * denom)));
}

// ============================================================
// SECTION 4: ray queries
// ============================================================

// Slab test: does the ray cross the box, and over what range of t?
// Narrows [*tmin, *tmax] to the part of the ray inside the box.
static bool rayBox(const float o[3], const float d[3],
                   const float bmin[3], const float bmax[3],
                   float* tmin, float* tmax)
{
    for (int a = 0; a < 3; a++) {
        if (fabsf(d[a]) < 1e-9f) {
            // Ray runs parallel to this pair of planes: it must start between them.
            if (o[a] < bmin[a] || o[a] > bmax[a]) return false;
        } else {
            float inv = 1.0f / d[a];
            float t1 = (bmin[a] - o[a]) * inv;
            float t2 = (bmax[a] - o[a]) * inv;
            if (t1 > t2) { float tmp = t1; t1 = t2; t2 = tmp; }
            if (t1 > *tmin) *tmin = t1;
            if (t2 < *tmax) *tmax = t2;
            if (*tmin > *tmax) return false;
        }
    }
    return true;
}

typedef struct {
    Vec3  o, d;          // Ray origin and (unit) direction
    float o3[3], d3[3];  // Same, as arrays so we can index by axis
    float bestT;         // Closest hit distance found so far
    int   bestTri;       // Which triangle that was (-1 = none yet)
} RayQuery;

// Walks the tree front-to-back along the ray.
//   [tmin, tmax] = the stretch of the ray that lies inside this node's region.
static void rayNode(const KDTree* t, int ni, RayQuery* q, float tmin, float tmax) {
    const KDNode* node = &t->nodes[ni];

    if (node->axis < 0) {
        // Leaf: test every triangle it holds.
        for (int i = 0; i < node->count; i++) {
            int id = t->indices[node->first + i];
            float hitT;
            if (rayTriangle(q->o, q->d, &t->tris[id], &hitT) && hitT < q->bestT) {
                q->bestT   = hitT;
                q->bestTri = id;
            }
        }
        return;
    }

    int   axis  = node->axis;
    float split = node->split;
    float o = q->o3[axis], d = q->d3[axis];

    // Which side of the plane does the ray START on? That side is "near".
    bool nearIsLeft = (o < split) || (o == split && d <= 0.0f);
    int  nearChild  = nearIsLeft ? node->left  : node->right;
    int  farChild   = nearIsLeft ? node->right : node->left;

    if (fabsf(d) < 1e-9f) {
        // Ray never crosses the plane: it only ever visits the near side.
        rayNode(t, nearChild, q, tmin, tmax);
        return;
    }

    float tPlane = (split - o) / d;   // Distance at which the ray crosses the plane

    if (tPlane >= tmax || tPlane < 0.0f) {
        rayNode(t, nearChild, q, tmin, tmax);       // Crossing is beyond our range
    } else if (tPlane <= tmin) {
        rayNode(t, farChild, q, tmin, tmax);        // Already crossed before we got here
    } else {
        rayNode(t, nearChild, q, tmin, tPlane);     // Visit near half first...
        if (q->bestT > tPlane)                      // ...and only look at the far half
            rayNode(t, farChild, q, tPlane, tmax);  // if no hit closer than the plane
    }
}

bool kdtreeRaycast(const KDTree* t, Vec3 origin, Vec3 dir, float maxDist, KDHit* hit) {
    if (!t->nodes) return false;

    float len = sqrtf(vDot(dir, dir));
    if (len < 1e-9f) return false;
    dir = vMul(dir, 1.0f / len);

    RayQuery q = { origin, dir,
                   { origin.x, origin.y, origin.z },
                   { dir.x, dir.y, dir.z },
                   maxDist, -1 };

    // Quick reject: clip the ray against the box around the whole mesh.
    const float pad = 1e-3f;
    float bmin[3] = { t->boundsMin.x - pad, t->boundsMin.y - pad, t->boundsMin.z - pad };
    float bmax[3] = { t->boundsMax.x + pad, t->boundsMax.y + pad, t->boundsMax.z + pad };
    float tmin = 0.0f, tmax = maxDist;
    if (!rayBox(q.o3, q.d3, bmin, bmax, &tmin, &tmax)) return false;

    rayNode(t, 0, &q, tmin, tmax);
    if (q.bestTri < 0) return false;

    if (hit) {
        hit->t      = q.bestT;
        hit->point  = vAdd(origin, vMul(dir, q.bestT));
        hit->normal = t->tris[q.bestTri].n;
    }
    return true;
}

// ============================================================
// SECTION 5: sphere queries
// ============================================================

typedef struct {
    Vec3  c;           // Sphere centre
    float r;           // Sphere radius
    float maxNY;       // Ignore triangles with |normal.y| above this (floors)
    bool  stopFirst;   // Overlap test: quit at the first touching triangle
    bool  hit;         // Did anything touch?
    float bestPen;     // Deepest penetration so far
    Vec3  bestPush;    // Push-out vector for that deepest penetration
} SphereQuery;

static void sphereNode(const KDTree* t, int ni, SphereQuery* q) {
    if (q->hit && q->stopFirst) return;
    const KDNode* node = &t->nodes[ni];

    if (node->axis < 0) {
        for (int i = 0; i < node->count; i++) {
            const KDTriangle* tri = &t->tris[t->indices[node->first + i]];

            // Floors/ceilings are skipped when the caller only wants walls.
            if (fabsf(tri->n.y) > q->maxNY) continue;

            Vec3  cp = closestPointOnTriangle(q->c, tri->a, tri->b, tri->c);
            Vec3  d  = vSub(q->c, cp);
            float d2 = vDot(d, d);
            if (d2 >= q->r * q->r) continue;          // Too far away: no touch

            float dist = sqrtf(d2);
            float pen  = q->r - dist;                 // How deep we've sunk in
            // Push straight away from the touched point. If the centre sits
            // exactly ON the surface the direction is undefined, so use the
            // triangle's own normal.
            Vec3 dir = (dist > 1e-6f) ? vMul(d, 1.0f / dist) : tri->n;

            q->hit = true;
            if (pen > q->bestPen) {
                q->bestPen  = pen;
                q->bestPush = vMul(dir, pen);
            }
            if (q->stopFirst) return;
        }
        return;
    }

    // Inner node: the sphere's extent along the split axis decides which
    // halves it can reach. Near the plane it may reach both.
    float centre = comp(q->c, node->axis);
    if (centre - q->r <= node->split) sphereNode(t, node->left,  q);
    if (centre + q->r >= node->split) sphereNode(t, node->right, q);
}

bool kdtreeSphereOverlap(const KDTree* t, Vec3 center, float radius) {
    if (!t->nodes) return false;
    SphereQuery q = { center, radius, 2.0f, true, false, 0.0f, {0,0,0} };
    sphereNode(t, 0, &q);
    return q.hit;
}

bool kdtreeSpherePush(const KDTree* t, Vec3 center, float radius,
                      float maxNormalY, Vec3* outPush)
{
    if (!t->nodes) return false;
    SphereQuery q = { center, radius, maxNormalY, false, false, 0.0f, {0,0,0} };
    sphereNode(t, 0, &q);
    if (q.hit && outPush) *outPush = q.bestPush;
    return q.hit;
}
