#include <gccore.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <malloc.h>
#include "common.h"
#include "kd_tree.h"
#include "island.h"
#include <stdint.h>
#include <float.h>

typedef struct {
    Vec3 position;
    float r, g, b;
} IslandVertex;

// Helper function to wrap around control point indices
static int clampCtrlIndex(int i) {
    int n = NUM_CTRL_POINTS;
    while (i < 0) i += n;
    while (i >= n) i -= n;
    return i;
}

// Catmull-Rom interpolation for smooth curves
static float catmullRom(float p0, float p1, float p2, float p3, float t) {
    float t2 = t * t;
    float t3 = t2 * t;
    return 0.5f * (
        (2 * p1) +
        (-p0 + p2) * t +
        (2 * p0 - 5 * p1 + 4 * p2 - p3) * t2 +
        (-p0 + 3 * p1 - 3 * p2 + p3) * t3
        );
}

// Generate random float between min and max
static float randomFloat(float min, float max) {
    return min + (max - min) * ((float)rand() / RAND_MAX);
}

// New helper functions for more varied island shapes
static float noise1D(float x, float freq, int octaves) {
    float value = 0.0f;
    float amplitude = 1.0f;
    float maxAmplitude = 0.0f;

    for (int i = 0; i < octaves; i++) {
        value += sinf(x * freq) * amplitude;
        maxAmplitude += amplitude;
        amplitude *= 0.5f;
        freq *= 2.0f;
    }

    return value / maxAmplitude;
}

static float ridgeNoise(float x, float freq, int octaves) {
    float val = noise1D(x, freq, octaves);
    return 1.0f - 2.0f * fabsf(val);
}

static float randomPeakHeight(Island* island) {
    // Base height with random variation
    float baseHeight = randomFloat(ISLAND_MIN_HEIGHT, ISLAND_MAX_HEIGHT);

    // Add extra height variation (30% chance of being taller)
    if (rand() % 100 < 30) {
        baseHeight *= 1.5f + 0.5f * ((float)rand() / RAND_MAX);
    }

    return baseHeight;
}

// Simplified island shape generation
static void generateIslandShape(Island* island, float baseRadius) {
    // Determine island type (0 = normal, 1 = peaked, 2 = flat, 3 = crater)
    int islandType = rand() % 4;
    float peakAngle = 2.0f * M_PI * ((float)rand() / RAND_MAX); // Random peak position

    // Generate control points with more variation
    for (int i = 0; i < NUM_CTRL_POINTS; ++i) {
        float angle = 2.0f * M_PI * i / NUM_CTRL_POINTS;
        float distanceToPeak = fabsf(angle - peakAngle);
        if (distanceToPeak > M_PI) distanceToPeak = 2.0f * M_PI - distanceToPeak;

        // Base radius with more variation
        float radiusVariation = 0.5f + 0.5f * ridgeNoise(angle, 2.0f, 3);
        island->ctrlRadius[i] = randomFloat(ISLAND_MIN_RADIUS, ISLAND_MAX_RADIUS) * radiusVariation;

        // Height generation based on island type
        switch (islandType) {
        case 0: // Normal island
            island->ctrlHeight[i] = randomPeakHeight(island) *
                (0.7f + 0.3f * cosf(distanceToPeak * 2.0f));
            break;
        case 1: // Peaked island
            island->ctrlHeight[i] = randomPeakHeight(island) *
                (0.3f + 0.7f * (1.0f - distanceToPeak / M_PI));
            break;
        case 2: // Flat-topped island
            island->ctrlHeight[i] = randomPeakHeight(island) *
                (0.8f - 0.2f * distanceToPeak / M_PI);
            if (distanceToPeak < 0.3f) island->ctrlHeight[i] *= 1.1f;
            break;
        case 3: // Crater island
            island->ctrlHeight[i] = randomPeakHeight(island) *
                (0.5f + 0.5f * sinf(distanceToPeak * 3.0f));
            break;
        }

        // Add some noise to break up patterns
        island->ctrlHeight[i] *= 0.9f + 0.2f * ((float)rand() / RAND_MAX);
    }

    // Smooth the control points (less smoothing for more variation)
    for (int i = 0; i < NUM_CTRL_POINTS; ++i) {
        int prev = clampCtrlIndex(i - 1);
        int next = clampCtrlIndex(i + 1);
        island->ctrlRadius[i] = (island->ctrlRadius[prev] + 2.0f * island->ctrlRadius[i] + island->ctrlRadius[next]) / 4.0f;
        island->ctrlHeight[i] = (island->ctrlHeight[prev] + 2.0f * island->ctrlHeight[i] + island->ctrlHeight[next]) / 4.0f;
    }
}

// Get interpolated radius at angle theta
static float getInterpolatedRadius(Island* island, float theta) {
    float t = theta / (2.0f * M_PI) * NUM_CTRL_POINTS;
    int i1 = (int)floorf(t);
    float localT = t - i1;

    int i0 = clampCtrlIndex(i1 - 1);
    int i2 = clampCtrlIndex(i1 + 1);
    int i3 = clampCtrlIndex(i1 + 2);
    i1 = clampCtrlIndex(i1);

    return catmullRom(island->ctrlRadius[i0], island->ctrlRadius[i1],
        island->ctrlRadius[i2], island->ctrlRadius[i3], localT);
}

// Get interpolated height at angle theta
static float getInterpolatedHeight(Island* island, float theta) {
    float t = theta / (2.0f * M_PI) * NUM_CTRL_POINTS;
    int i1 = (int)floorf(t);
    float localT = t - i1;

    int i0 = clampCtrlIndex(i1 - 1);
    int i2 = clampCtrlIndex(i1 + 1);
    int i3 = clampCtrlIndex(i1 + 2);
    i1 = clampCtrlIndex(i1);

    return catmullRom(island->ctrlHeight[i0], island->ctrlHeight[i1],
        island->ctrlHeight[i2], island->ctrlHeight[i3], localT);
}

// Color based on height
static void colorForHeight(Island* island, float height, float* r, float* g, float* b) {
    // Define color transition thresholds in world-space Y-values
    float bottomColorTop = 1.2f;  // height where bottom color ends
    float middleColorTop = 4.5f;  // height where middle color ends

    float y = height;

    switch (island->colorStyle) {
    case ISLAND_TROPICAL:
        if (y < bottomColorTop) {
            *r = 0.95f; *g = 0.85f; *b = 0.6f;  // Sand
        }
        else if (y < middleColorTop) {
            *r = 0.2f; *g = 0.7f; *b = 0.3f;  // Grass
        }
        else {
            *r = 0.0f; *g = 1.0f; *b = 0.0f;  // Bright green
        }
        break;
    case ISLAND_VOLCANO:
        if (y < bottomColorTop) {
            *r = 0.2f; *g = 0.1f; *b = 0.0f;  // Dark brown
        }
        else if (y < middleColorTop) {
            *r = 0.3f; *g = 0.3f; *b = 0.3f;  // Gray
        }
        else {
            *r = 0.8f; *g = 0.2f; *b = 0.1f;  // Red
        }
        break;
    case ISLAND_ARCTIC:
        if (y < bottomColorTop) {
            *r = 0.8f; *g = 0.9f; *b = 1.0f;  // Light blue
        }
        else if (y < middleColorTop) {
            *r = 0.7f; *g = 0.8f; *b = 0.9f;  // Blue-gray
        }
        else {
            *r = 0.6f; *g = 0.6f; *b = 1.0f;  // Dark gray
        }
        break;
    }
}


void initIsland(Island* island, float baseRadius) {
    if (island->isInitialized) return;

    // Seed RNG with unique value for each island
    unsigned int seed = (unsigned int)time(NULL) ^ (uintptr_t)island;
    srand(seed);

    // More color style variation
    island->colorStyle = rand() % 3;
    if (rand() % 5 == 0) { // 20% chance of special color style
        island->colorStyle = rand() % 3; // Re-roll for more variation
    }

    generateIslandShape(island, baseRadius);

    // Calculate number of vertices needed
    island->numVertices = NUM_SEGMENTS * (NUM_SEGMENTS / 2) * 4;
    island->vertices = (IslandVertex*)memalign(32, island->numVertices * sizeof(IslandVertex));

    int vertexIndex = 0;

    for (int i = 0; i < NUM_SEGMENTS; ++i) {
        float theta1 = (i * 2 * M_PI) / NUM_SEGMENTS;
        float theta2 = ((i + 1) * 2 * M_PI) / NUM_SEGMENTS;

        for (int j = 0; j < NUM_SEGMENTS / 2; ++j) {
            float phi1 = (j * M_PI) / NUM_SEGMENTS - M_PI / 2;
            float phi2 = ((j + 1) * M_PI) / NUM_SEGMENTS - M_PI / 2;

            float cosPhi1 = cosf(phi1);
            float cosPhi2 = cosf(phi2);

            // Calculate radius at each point
            float r11 = getInterpolatedRadius(island, theta1) * cosPhi1;
            float r12 = getInterpolatedRadius(island, theta2) * cosPhi1;
            float r22 = getInterpolatedRadius(island, theta2) * cosPhi2;
            float r21 = getInterpolatedRadius(island, theta1) * cosPhi2;

            // Get height offsets
            float hOffset1 = getInterpolatedHeight(island, theta1);
            float hOffset2 = getInterpolatedHeight(island, theta2);

            // Base shape is a hemisphere that flattens at the bottom
            float baseShape1 = (1.0f - cosPhi1 * cosPhi1);  // Flatter at bottom
            float baseShape2 = (1.0f - cosPhi2 * cosPhi2);

            // Vertex 1
            IslandVertex* v = &((IslandVertex*)island->vertices)[vertexIndex++];
            v->position.x = island->position.x + r11 * cosf(theta1);
            v->position.y = island->position.y + baseShape1 * hOffset1;
            v->position.z = island->position.z + r11 * sinf(theta1);
            colorForHeight(island, v->position.y, &v->r, &v->g, &v->b);

            // Vertex 2
            v = &((IslandVertex*)island->vertices)[vertexIndex++];
            v->position.x = island->position.x + r12 * cosf(theta2);
            v->position.y = island->position.y + baseShape1 * hOffset2;
            v->position.z = island->position.z + r12 * sinf(theta2);
            colorForHeight(island, v->position.y, &v->r, &v->g, &v->b);

            // Vertex 3
            v = &((IslandVertex*)island->vertices)[vertexIndex++];
            v->position.x = island->position.x + r22 * cosf(theta2);
            v->position.y = island->position.y + baseShape2 * hOffset2;
            v->position.z = island->position.z + r22 * sinf(theta2);
            colorForHeight(island, v->position.y, &v->r, &v->g, &v->b);

            // Vertex 4
            v = &((IslandVertex*)island->vertices)[vertexIndex++];
            v->position.x = island->position.x + r21 * cosf(theta1);
            v->position.y = island->position.y + baseShape2 * hOffset1;
            v->position.z = island->position.z + r21 * sinf(theta1);
            colorForHeight(island, v->position.y, &v->r, &v->g, &v->b);

            // Create collision triangles
            Triangle tri1 = {
                ((IslandVertex*)island->vertices)[vertexIndex - 4].position,
                ((IslandVertex*)island->vertices)[vertexIndex - 3].position,
                ((IslandVertex*)island->vertices)[vertexIndex - 2].position
            };

            Triangle tri2 = {
                ((IslandVertex*)island->vertices)[vertexIndex - 4].position,
                ((IslandVertex*)island->vertices)[vertexIndex - 2].position,
                ((IslandVertex*)island->vertices)[vertexIndex - 1].position
            };

            island->kdTree = kd_insert(island->kdTree, tri1, 0);
            island->kdTree = kd_insert(island->kdTree, tri2, 0);
        }
    }

    island->isInitialized = true;
}

void drawIsland(Island* island) {
    if (!island->isInitialized) return;

    // Draw all quads
    for (int i = 0; i < island->numVertices; i += 4) {
        GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
        for (int j = 0; j < 4; j++) {
            IslandVertex* v = &((IslandVertex*)island->vertices)[i + j];
            GX_Position3f32(v->position.x, v->position.y, v->position.z);
            GX_Color3f32(v->r, v->g, v->b);
        }
        GX_End();
    }
}


// Helper function: Find closest point on triangle to a point
static Vec3 closestPointOnTriangle(Vec3 p, Triangle tri) {
    Vec3 a = tri.v1, b = tri.v2, c = tri.v3;

    Vec3 ab = { b.x - a.x, b.y - a.y, b.z - a.z };
    Vec3 ac = { c.x - a.x, c.y - a.y, c.z - a.z };
    Vec3 ap = { p.x - a.x, p.y - a.y, p.z - a.z };

    float d1 = ab.x * ap.x + ab.y * ap.y + ab.z * ap.z;
    float d2 = ac.x * ap.x + ac.y * ap.y + ac.z * ap.z;

    if (d1 <= 0.0f && d2 <= 0.0f) return a;

    Vec3 bp = { p.x - b.x, p.y - b.y, p.z - b.z };
    float d3 = ab.x * bp.x + ab.y * bp.y + ab.z * bp.z;
    float d4 = ac.x * bp.x + ac.y * bp.y + ac.z * bp.z;
    if (d3 >= 0.0f && d4 <= d3) return b;

    float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
        float v = d1 / (d1 - d3);
        Vec3 result = {
            a.x + v * ab.x,
            a.y + v * ab.y,
            a.z + v * ab.z
        };
        return result;
    }

    Vec3 cp = { p.x - c.x, p.y - c.y, p.z - c.z };
    float d5 = ab.x * cp.x + ab.y * cp.y + ab.z * cp.z;
    float d6 = ac.x * cp.x + ac.y * cp.y + ac.z * cp.z;
    if (d6 >= 0.0f && d5 <= d6) return c;

    float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
        float w = d2 / (d2 - d6);
        Vec3 result = {
            a.x + w * ac.x,
            a.y + w * ac.y,
            a.z + w * ac.z
        };
        return result;
    }

    float va = d3 * d6 - d5 * d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
        float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        Vec3 result = {
            b.x + w * (c.x - b.x),
            b.y + w * (c.y - b.y),
            b.z + w * (c.z - b.z)
        };
        return result;
    }

    float denom = 1.0f / (va + vb + vc);
    float v = vb * denom;
    float w = vc * denom;

    Vec3 result = {
        a.x + ab.x * v + ac.x * w,
        a.y + ab.y * v + ac.y * w,
        a.z + ab.z * v + ac.z * w
    };
    return result;
}

/**
 * Checks if a sphere at `position` with radius `radius` collides with the island mesh.
 * Uses the island's KD-tree to query nearby triangles for collision detection.
 * 
 * @param island Pointer to the Island struct containing geometry and KD-tree.
 * @param position Center position of the sphere to test collision.
 * @param radius Radius of the sphere.
 * @return true if collision occurs, false otherwise.
 */
bool checkIslandCollision(Island* island, Vec3 position, float radius) {
    // If KD-tree is not built yet, can't check collisions
    if (!island->kdTree) return false;

    // Preallocate array to store nearby triangles
    int triTotal = 32;  // Adjust this size depending on your expected density
    Triangle nearbyTris[triTotal];
    int triCount = 0;

    // Smaller objects break the math, so pretend the objects have radius = 1
    float workingRadius = 1.0f;

    // Collision search radius is doubled here to ensure some margin
    float collisionRadius = workingRadius * 2.0f;

    // Callback function to collect triangles found in KD-tree query
    void collectCallback(const Triangle* tri) {
        if (triCount < triTotal) {
            nearbyTris[triCount++] = *tri;
        }
    }

    // Query KD-tree for all triangles within collisionRadius of position
    kd_query_nearest(island->kdTree, position, collisionRadius, collectCallback);

    // For each nearby triangle, test actual collision against the sphere
    for (int i = 0; i < triCount; i++) {
        Triangle tri = nearbyTris[i];

        // Find closest point on triangle to sphere center
        Vec3 closest = closestPointOnTriangle(position, tri);

        // Compute squared distance between sphere center and closest point
        float dx = position.x - closest.x;
        float dy = position.y - closest.y;
        float dz = position.z - closest.z;
        float distSq = dx * dx + dy * dy + dz * dz;

        // If the distance is less than or equal to radius, collision occurs
        // See if in the actual radius
        if (radius >= 1) {
            if (distSq <= radius * radius) {
                return true;
            }
        }
        else {
            if (distSq <= radius) {
                return true;
            }
        }
        
    }

    // No collision found
    return false;
}

// Helper: normalize a vector
static Vec3 normalize(Vec3 v) {
    float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    return (Vec3) { v.x / len, v.y / len, v.z / len };
}

// Helper: subtract two vectors
static Vec3 subtract(Vec3 a, Vec3 b) {
    return (Vec3) { a.x - b.x, a.y - b.y, a.z - b.z };
}

// Helper: dot product
static float dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

// Helper: cross product
static Vec3 cross(Vec3 a, Vec3 b) {
    return (Vec3) {
        a.y* b.z - a.z * b.y,
            a.z* b.x - a.x * b.z,
            a.x* b.y - a.y * b.x
    };
}

// Ray-triangle intersection (Möller–Trumbore algorithm)
static bool ray_intersects_triangle(Vec3 orig, Vec3 dir, const Triangle* tri, float maxDist) {
    const float EPSILON = 1e-6f;
    Vec3 v0v1 = subtract(tri->v2, tri->v1);
    Vec3 v0v2 = subtract(tri->v3, tri->v1);
    Vec3 pvec = cross(dir, v0v2);
    float det = dot(v0v1, pvec);

    if (fabsf(det) < EPSILON) return false;

    float invDet = 1.0f / det;
    Vec3 tvec = subtract(orig, tri->v1);
    float u = dot(tvec, pvec) * invDet;
    if (u < 0 || u > 1) return false;

    Vec3 qvec = cross(tvec, v0v1);
    float v = dot(dir, qvec) * invDet;
    if (v < 0 || u + v > 1) return false;

    float t = dot(v0v2, qvec) * invDet;
    return (t > EPSILON && t < maxDist);
}

// Context for KD-tree callback
typedef struct {
    Vec3 origin;
    Vec3 direction;
    float maxDist;
    bool blocked;
} RayCheckContext;

// Static callback wrapper
static RayCheckContext* ray_context = NULL;

static void ray_query_callback(const Triangle* tri) {
    if (ray_intersects_triangle(ray_context->origin, ray_context->direction, tri, ray_context->maxDist)) {
        ray_context->blocked = true;
    }
}

bool cameraCoveredCheck(Vec3 cameraPos, Vec3 playerPos, Island* island) {
    if (!island || !island->kdTree) return false;

    Vec3 dir = subtract(playerPos, cameraPos);
    float distance = sqrtf(dot(dir, dir));
    dir = normalize(dir);

    RayCheckContext context = {
        .origin = cameraPos,
        .direction = dir,
        .maxDist = distance,
        .blocked = false
    };

    ray_context = &context;

    // Use a query radius slightly larger than the ray width
    float queryRadius = 1.0f;

    kd_query_nearest(island->kdTree, cameraPos, distance, ray_query_callback);

    ray_context = NULL;

    return context.blocked;
}


/*

Draw red what the player is colliding with and make it 
closer to player so it isn't inside the island/whatever
red = no collision, green means collision
*/
// Helper function: Clamp a value between 0 and 1
static float clamp01(float x) {
    return (x < 0) ? 0 : (x > 1) ? 1 : x;
}

void drawNearestTriangleToPlayer(Island* island, Vec3 playerPos, float playerRadius) {
    if (!island || !island->kdTree) return;

    Triangle nearestTri;
    bool found = false;
    float nearestDistSq = FLT_MAX;

    void findNearestCallback(const Triangle * tri) {
        // Compute full 3D center of triangle
        Vec3 center = {
            (tri->v1.x + tri->v2.x + tri->v3.x) / 3.0f,
            (tri->v1.y + tri->v2.y + tri->v3.y) / 3.0f,
            (tri->v1.z + tri->v2.z + tri->v3.z) / 3.0f
        };

        float dx = playerPos.x - center.x;
        float dy = playerPos.y - center.y;
        float dz = playerPos.z - center.z;
        float distSq = dx * dx + dy * dy + dz * dz;

        if (distSq < nearestDistSq) {
            nearestDistSq = distSq;
            nearestTri = *tri;
            found = true;
        }
    }

    kd_query_nearest(island->kdTree, playerPos, playerRadius * 3.0f, findNearestCallback);
    if (!found) return;

    // Compute closest point on nearest triangle to player
    Vec3 closestPoint = closestPointOnTriangle(playerPos, nearestTri);

    // Compute triangle normal
    Vec3 u = {
        nearestTri.v2.x - nearestTri.v1.x,
        nearestTri.v2.y - nearestTri.v1.y,
        nearestTri.v2.z - nearestTri.v1.z
    };
    Vec3 v = {
        nearestTri.v3.x - nearestTri.v1.x,
        nearestTri.v3.y - nearestTri.v1.y,
        nearestTri.v3.z - nearestTri.v1.z
    };
    Vec3 normal = {
        u.y * v.z - u.z * v.y,
        u.z * v.x - u.x * v.z,
        u.x * v.y - u.y * v.x
    };
    float length = sqrtf(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
    if (length > 0.0f) {
        normal.x /= length;
        normal.y /= length;
        normal.z /= length;
    }
    else {
        normal = (Vec3){ 0,1,0 }; // fallback normal
    }

    // Offset cube slightly above the triangle surface (along normal)
    float offsetDist = 0.1f;
    Vec3 cubeCenter = {
        closestPoint.x + normal.x * offsetDist,
        closestPoint.y + normal.y * offsetDist,
        closestPoint.z + normal.z * offsetDist
    };

    // Color based on actual collision
    bool collision = checkIslandCollision(island, playerPos, playerRadius);
    float r = collision ? 0.0f : 1.0f;
    float g = collision ? 1.0f : 0.0f;
    float b = 0.0f;

    // Draw small cube at cubeCenter
    float halfSize = 0.1f;

    static const int faceIndices[6][4] = {
        {0, 1, 2, 3}, // front
        {4, 5, 6, 7}, // back
        {0, 1, 5, 4}, // bottom
        {2, 3, 7, 6}, // top
        {1, 2, 6, 5}, // right
        {0, 3, 7, 4}  // left
    };

    static const Vec3 offsets[8] = {
        {-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
        {-1, -1, 1},  {1, -1, 1},  {1, 1, 1},  {-1, 1, 1}
    };

    for (int i = 0; i < 6; i++) {
        GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
        for (int j = 0; j < 4; ++j) {
            Vec3 o = offsets[faceIndices[i][j]];
            GX_Position3f32(
                cubeCenter.x + o.x * halfSize,
                cubeCenter.y + o.y * halfSize,
                cubeCenter.z + o.z * halfSize
            );
            GX_Color3f32(r, g, b);
        }
        GX_End();
    }
}



// Simply frees the memory
void freeIslandResources(Island* island) {
    if (!island) return;

    if (island->kdTree) {
        kd_free(island->kdTree);
        island->kdTree = NULL;
    }

    if (island->vertices) {
        free(island->vertices);
        island->vertices = NULL;
    }

    island->isInitialized = false;
}