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

bool checkIslandCollision(Island* island, Vec3 position, float radius) {
    if (!island->kdTree) return false;

    Triangle nearbyTris[16];
    int triCount = 0;
    float collisionRadius = radius * 1.5f;

    void collectCallback(const Triangle * tri) {
        if (triCount < 16) {
            nearbyTris[triCount++] = *tri;
        }
    }

    kd_query_nearest(island->kdTree, position, collisionRadius * 2.0f, collectCallback);

    for (int i = 0; i < triCount; i++) {
        Triangle tri = nearbyTris[i];
        float dx1 = position.x - tri.v1.x;
        float dy1 = position.y - tri.v1.y;
        float dz1 = position.z - tri.v1.z;
        float distSq1 = dx1 * dx1 + dy1 * dy1 + dz1 * dz1;

        float dx2 = position.x - tri.v2.x;
        float dy2 = position.y - tri.v2.y;
        float dz2 = position.z - tri.v2.z;
        float distSq2 = dx2 * dx2 + dy2 * dy2 + dz2 * dz2;

        float dx3 = position.x - tri.v3.x;
        float dy3 = position.y - tri.v3.y;
        float dz3 = position.z - tri.v3.z;
        float distSq3 = dx3 * dx3 + dy3 * dy3 + dz3 * dz3;

        float minDistSq = fminf(fminf(distSq1, distSq2), distSq3);
        if (minDistSq <= (radius * radius)) {
            return true;
        }

        Vec3 center = {
            (tri.v1.x + tri.v2.x + tri.v3.x) / 3.0f,
            (tri.v1.y + tri.v2.y + tri.v3.y) / 3.0f,
            (tri.v1.z + tri.v2.z + tri.v3.z) / 3.0f
        };

        float dxc = position.x - center.x;
        float dyc = position.y - center.y;
        float dzc = position.z - center.z;
        float centerDistSq = dxc * dxc + dyc * dyc + dzc * dzc;

        if (centerDistSq <= (radius * 1.2f * radius * 1.2f)) {
            return true;
        }
    }

    return false;
}


/*

Finds the three closest vertices to the player
marks them with red cubes

then draws a '2d' triangle connecting them (filled)
red means no collision while green means collision (the player is in that area, not necessarity touching a vertex)
*/
void drawNearestTriangleToPlayer(Island* island, Vec3 playerPos, float playerRadius) {
    if (!island || !island->vertices || island->numVertices < 3) return;

    IslandVertex* verts = (IslandVertex*)island->vertices;

    int closestIndices[3] = { 0, 1, 2 };
    float closestDist = FLT_MAX;

    // Find closest triangle by centroid distance
    for (int i = 0; i < island->numVertices - 2; i += 3) {
        Vec3 a = verts[i].position;
        Vec3 b = verts[i + 1].position;
        Vec3 c = verts[i + 2].position;

        Vec3 centroid = {
            (a.x + b.x + c.x) / 3.0f,
            (a.y + b.y + c.y) / 3.0f,
            (a.z + b.z + c.z) / 3.0f
        };

        float dx = playerPos.x - centroid.x;
        float dy = playerPos.y - centroid.y;
        float dz = playerPos.z - centroid.z;
        float distSq = dx * dx + dy * dy + dz * dz;

        if (distSq < closestDist) {
            closestDist = distSq;
            closestIndices[0] = i;
            closestIndices[1] = i + 1;
            closestIndices[2] = i + 2;
        }
    }

    // Extract vertices of closest triangle
    Vec3 v1 = verts[closestIndices[0]].position;
    Vec3 v2 = verts[closestIndices[1]].position;
    Vec3 v3 = verts[closestIndices[2]].position;

    // Calculate centroid
    Vec3 centroid = {
        (v1.x + v2.x + v3.x) / 3.0f,
        (v1.y + v2.y + v3.y) / 3.0f,
        (v1.z + v2.z + v3.z) / 3.0f
    };

    // Vector from centroid to player
    Vec3 dir = {
        playerPos.x - centroid.x,
        playerPos.y - centroid.y,
        playerPos.z - centroid.z
    };

    // Normalize dir
    float length = sqrtf(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
    if (length > 0.0001f) {
        dir.x /= length;
        dir.y /= length;
        dir.z /= length;
    }
    else {
        // Avoid zero length direction
        dir.x = 0.0f;
        dir.y = 0.0f;
        dir.z = 0.0f;
    }

    // Amount to offset triangle along dir vector
    const float offsetAmount = 1.0f; // Adjust this value as needed

    // Shifted vertices towards player
    Vec3 triToward[3] = {
        { v1.x + dir.x * offsetAmount, v1.y + dir.y * offsetAmount, v1.z + dir.z * offsetAmount },
        { v2.x + dir.x * offsetAmount, v2.y + dir.y * offsetAmount, v2.z + dir.z * offsetAmount },
        { v3.x + dir.x * offsetAmount, v3.y + dir.y * offsetAmount, v3.z + dir.z * offsetAmount }
    };

    // Shifted vertices away from player
    Vec3 triAway[3] = {
        { v1.x - dir.x * offsetAmount, v1.y - dir.y * offsetAmount, v1.z - dir.z * offsetAmount },
        { v2.x - dir.x * offsetAmount, v2.y - dir.y * offsetAmount, v2.z - dir.z * offsetAmount },
        { v3.x - dir.x * offsetAmount, v3.y - dir.y * offsetAmount, v3.z - dir.z * offsetAmount }
    };

    // Helper function to check collision with triangle (use your existing logic or a helper)
    bool collidesToward = false;
    bool collidesAway = false;

    // Check collision helper — you can adapt your existing collision function for single triangle:
    bool checkCollisionTriangle(Vec3 p, float radius, Vec3 t[3]) {
        // Simple distance check from player position to triangle vertices and centroid (basic proxy)
        for (int i = 0; i < 3; i++) {
            float dx = p.x - t[i].x;
            float dy = p.y - t[i].y;
            float dz = p.z - t[i].z;
            float distSq = dx * dx + dy * dy + dz * dz;
            if (distSq <= radius * radius) return true;
        }
        Vec3 center = {
            (t[0].x + t[1].x + t[2].x) / 3.0f,
            (t[0].y + t[1].y + t[2].y) / 3.0f,
            (t[0].z + t[1].z + t[2].z) / 3.0f
        };
        float dx = p.x - center.x;
        float dy = p.y - center.y;
        float dz = p.z - center.z;
        float distSq = dx * dx + dy * dy + dz * dz;
        return distSq <= radius * radius * 1.2f * 1.2f;
    }

    collidesToward = checkCollisionTriangle(playerPos, playerRadius, triToward);
    collidesAway = checkCollisionTriangle(playerPos, playerRadius, triAway);

    bool colliding = collidesToward || collidesAway;

    // Draw both triangles, green if colliding, red if not
    GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 3);
    for (int i = 0; i < 3; i++) {
        GX_Position3f32(triToward[i].x, triToward[i].y, triToward[i].z);
        if (colliding)
            GX_Color3f32(0.0f, 1.0f, 0.0f); // Green
        else
            GX_Color3f32(1.0f, 0.0f, 0.0f); // Red
    }
    GX_End();

    GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 3);
    for (int i = 0; i < 3; i++) {
        GX_Position3f32(triAway[i].x, triAway[i].y, triAway[i].z);
        if (colliding)
            GX_Color3f32(0.0f, 1.0f, 0.0f); // Green
        else
            GX_Color3f32(1.0f, 0.0f, 0.0f); // Red
    }
    GX_End();
}





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