#include <gccore.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <malloc.h>  // Added for memalign
#include "common.h"
#include "kd_tree.h"
#include "island.h"
#include <time.h>
#include <stdint.h>

typedef struct {
    Vec3 position;
    float r, g, b;
} IslandVertex;

static bool isInitialized = false;
static float ctrlRadius[NUM_CTRL_POINTS];
static float ctrlHeight[NUM_CTRL_POINTS];
static KDNode* islandKDTree = NULL;
static IslandVertex* islandVertices = NULL;
static int numVertices = 0;
static IslandType islandColorStyle;
static Vec3 islandPosition = { 0.0f, 0.0f, 15.0f };

// Helper functions
static int clampCtrlIndex(int i) {
    int n = NUM_CTRL_POINTS;
    while (i < 0) i += n;
    while (i >= n) i -= n;
    return i;
}

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

static float randomFloat(float min, float max) {
    return min + (max - min) * ((float)rand() / RAND_MAX);
}

static void generateIslandShape(float baseRadius) {
    // Only initialize random seed once at program start
    static bool firstTime = true;
    if (firstTime) {
        srand(time(NULL));
        firstTime = false;
    }

    // More varied island types
    islandColorStyle = rand() % 3;

    // More parameters for shape variation
    float noiseFreq1 = randomFloat(1.0f, 3.0f);
    float noiseFreq2 = randomFloat(0.3f, 1.5f);
    float noiseAmp1 = randomFloat(0.1f, 0.5f);
    float noiseAmp2 = randomFloat(0.05f, 0.2f);
    float heightScale = randomFloat(1.0f, 3.0f);
    float asymmetry = randomFloat(0.5f, 2.0f); // Makes islands lopsided

    // Generate control points with more complex noise
    for (int i = 0; i < NUM_CTRL_POINTS; ++i) {
        float angle = 2.0f * M_PI * i / NUM_CTRL_POINTS;

        // Multiple layers of noise
        float wave1 = sinf(noiseFreq1 * angle);
        float wave2 = cosf(noiseFreq2 * angle * asymmetry);
        float wave3 = sinf(noiseFreq1 * 2.3f * angle + 1.5f);

        // Combine waves with different weights
        float combined = (wave1 * 0.5f + wave2 * 0.3f + wave3 * 0.2f);

        // Add some pure randomness
        float jitter = randomFloat(-0.3f, 0.3f);

        // Calculate radius with more variation
        ctrlRadius[i] = baseRadius * (1.0f + noiseAmp1 * combined + jitter);

        // Height with more variation
        float heightBase = sinf(angle * heightScale);
        float heightMod = cosf(angle * 0.7f * asymmetry);
        ctrlHeight[i] = heightBase * (1.0f + noiseAmp2 * heightMod) + randomFloat(-0.5f, 0.5f);

        // Add occasional spikes for more interesting shapes
        if (rand() % 10 == 0) {
            ctrlHeight[i] += randomFloat(0.5f, 2.0f);
        }
    }

    // Smooth the control points a bit
    for (int i = 0; i < NUM_CTRL_POINTS; ++i) {
        int prev = clampCtrlIndex(i - 1);
        int next = clampCtrlIndex(i + 1);
        ctrlRadius[i] = (ctrlRadius[prev] + ctrlRadius[i] + ctrlRadius[next]) / 3.0f;
        ctrlHeight[i] = (ctrlHeight[prev] + ctrlHeight[i] + ctrlHeight[next]) / 3.0f;
    }
}



static float getInterpolatedRadius(float theta) {
    float t = theta / (2.0f * M_PI) * NUM_CTRL_POINTS;
    int i1 = (int)floorf(t);
    float localT = t - i1;

    int i0 = clampCtrlIndex(i1 - 1);
    int i2 = clampCtrlIndex(i1 + 1);
    int i3 = clampCtrlIndex(i1 + 2);
    i1 = clampCtrlIndex(i1);

    return catmullRom(ctrlRadius[i0], ctrlRadius[i1], ctrlRadius[i2], ctrlRadius[i3], localT);
}

static float getInterpolatedHeight(float theta) {
    float t = theta / (2.0f * M_PI) * NUM_CTRL_POINTS;
    int i1 = (int)floorf(t);
    float localT = t - i1;

    int i0 = clampCtrlIndex(i1 - 1);
    int i2 = clampCtrlIndex(i1 + 1);
    int i3 = clampCtrlIndex(i1 + 2);
    i1 = clampCtrlIndex(i1);

    return catmullRom(ctrlHeight[i0], ctrlHeight[i1], ctrlHeight[i2], ctrlHeight[i3], localT);
}

static void colorForHeight(float height, float baseY, float* r, float* g, float* b) {
    float t = (height - baseY) / 3.0f;

    switch (islandColorStyle) {
    case ISLAND_TROPICAL:
        if (t < 0.2f) { *r = 0.95f; *g = 0.85f; *b = 0.6f; }
        else if (t < 0.6f) { *r = 0.2f; *g = 0.7f; *b = 0.3f; }
        else { *r = 0.0f; *g = 1.0f; *b = 0.0f; }
        break;
    case ISLAND_VOLCANO:
        if (t < 0.3f) { *r = 0.2f; *g = 0.1f; *b = 0.0f; }
        else if (t < 0.6f) { *r = 0.3f; *g = 0.3f; *b = 0.3f; }
        else { *r = 0.8f; *g = 0.2f; *b = 0.1f; }
        break;
    case ISLAND_ARCTIC:
        if (t < 0.3f) { *r = 0.8f; *g = 0.9f; *b = 1.0f; }
        else if (t < 0.6f) { *r = 0.7f; *g = 0.8f; *b = 0.9f; }
        else { *r = 0.4f; *g = 0.4f; *b = 0.5f; }
        break;
    }
}

void initIsland(float baseRadius) {
    if (isInitialized) return;

    unsigned int seed = (unsigned int)time(NULL) ^ (uintptr_t)&baseRadius;
    srand(seed);
    // Better: call this once in your game loop after user interaction, etc.
    srand(time(NULL) ^ ((uintptr_t)&seed << 16) ^ (uintptr_t)&ctrlRadius);

    generateIslandShape(baseRadius);

    // Calculate number of vertices needed (4 per quad, NUM_SEGMENTS^2 quads)
    numVertices = NUM_SEGMENTS * (NUM_SEGMENTS / 2) * 4;
    islandVertices = (IslandVertex*)memalign(32, numVertices * sizeof(IslandVertex));

    int vertexIndex = 0;

    for (int i = 0; i < NUM_SEGMENTS; ++i) {
        float theta1 = (i * 2 * M_PI) / NUM_SEGMENTS;
        float theta2 = ((i + 1) * 2 * M_PI) / NUM_SEGMENTS;

        for (int j = 0; j < NUM_SEGMENTS / 2; ++j) {
            float phi1 = (j * M_PI) / NUM_SEGMENTS - M_PI / 2;
            float phi2 = ((j + 1) * M_PI) / NUM_SEGMENTS - M_PI / 2;

            float cosPhi1 = cosf(phi1);
            float cosPhi2 = cosf(phi2);

            float r11 = getInterpolatedRadius(theta1) * cosPhi1;
            float r12 = getInterpolatedRadius(theta2) * cosPhi1;
            float r22 = getInterpolatedRadius(theta2) * cosPhi2;
            float r21 = getInterpolatedRadius(theta1) * cosPhi2;

            float hOffset1 = getInterpolatedHeight(theta1);
            float hOffset2 = getInterpolatedHeight(theta2);

            float baseHill1 = 2.0f * (1.0f - cosPhi1 * cosPhi1);
            float baseHill2 = 2.0f * (1.0f - cosPhi2 * cosPhi2);

            // Vertex 1
            islandVertices[vertexIndex].position.x = islandPosition.x + r11 * cosf(theta1);
            islandVertices[vertexIndex].position.y = islandPosition.y + baseHill1 + hOffset1;
            islandVertices[vertexIndex].position.z = islandPosition.z + r11 * sinf(theta1);
            colorForHeight(islandVertices[vertexIndex].position.y, islandPosition.y,
                &islandVertices[vertexIndex].r,
                &islandVertices[vertexIndex].g,
                &islandVertices[vertexIndex].b);
            vertexIndex++;

            // Vertex 2
            islandVertices[vertexIndex].position.x = islandPosition.x + r12 * cosf(theta2);
            islandVertices[vertexIndex].position.y = islandPosition.y + baseHill1 + hOffset2;
            islandVertices[vertexIndex].position.z = islandPosition.z + r12 * sinf(theta2);
            colorForHeight(islandVertices[vertexIndex].position.y, islandPosition.y,
                &islandVertices[vertexIndex].r,
                &islandVertices[vertexIndex].g,
                &islandVertices[vertexIndex].b);
            vertexIndex++;

            // Vertex 3
            islandVertices[vertexIndex].position.x = islandPosition.x + r22 * cosf(theta2);
            islandVertices[vertexIndex].position.y = islandPosition.y + baseHill2 + hOffset2;
            islandVertices[vertexIndex].position.z = islandPosition.z + r22 * sinf(theta2);
            colorForHeight(islandVertices[vertexIndex].position.y, islandPosition.y,
                &islandVertices[vertexIndex].r,
                &islandVertices[vertexIndex].g,
                &islandVertices[vertexIndex].b);
            vertexIndex++;

            // Vertex 4
            islandVertices[vertexIndex].position.x = islandPosition.x + r21 * cosf(theta1);
            islandVertices[vertexIndex].position.y = islandPosition.y + baseHill2 + hOffset1;
            islandVertices[vertexIndex].position.z = islandPosition.z + r21 * sinf(theta1);
            colorForHeight(islandVertices[vertexIndex].position.y, islandPosition.y,
                &islandVertices[vertexIndex].r,
                &islandVertices[vertexIndex].g,
                &islandVertices[vertexIndex].b);
            vertexIndex++;

            // Create collision triangles
            Triangle tri1 = {
                islandVertices[vertexIndex - 4].position,
                islandVertices[vertexIndex - 3].position,
                islandVertices[vertexIndex - 2].position
            };

            Triangle tri2 = {
                islandVertices[vertexIndex - 4].position,
                islandVertices[vertexIndex - 2].position,
                islandVertices[vertexIndex - 1].position
            };

            islandKDTree = kd_insert(islandKDTree, tri1, 0);
            islandKDTree = kd_insert(islandKDTree, tri2, 0);
        }
    }

    isInitialized = true;
}

void drawIsland() {
    if (!isInitialized) return;

    // Draw all quads
    for (int i = 0; i < numVertices; i += 4) {
        GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
        for (int j = 0; j < 4; j++) {
            IslandVertex* v = &islandVertices[i + j];
            GX_Position3f32(v->position.x, v->position.y, v->position.z);
            GX_Color3f32(v->r, v->g, v->b);
        }
        GX_End();
    }
}

bool checkIslandCollision(Vec3 position, float radius) {
    if (!islandKDTree) return false;

    Triangle nearbyTris[16];
    int triCount = 0;
    float collisionRadius = radius * 1.5f;

    void collectCallback(const Triangle * tri) {
        if (triCount < 16) {
            nearbyTris[triCount++] = *tri;
        }
    }

    kd_query_nearest(islandKDTree, position, collisionRadius * 2.0f, collectCallback);

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

void freeIslandResources() {
    if (islandKDTree) {
        kd_free(islandKDTree);
        islandKDTree = NULL;
    }
    if (islandVertices) {
        free(islandVertices);
        islandVertices = NULL;
    }
    isInitialized = false;
}