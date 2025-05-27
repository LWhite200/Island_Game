#include "manager.h"
#include <stdlib.h>
#include <time.h>
#include <string.h>

void initIslandManager(IslandManager* manager) {
    manager->count = 0;
    memset(manager->islands, 0, sizeof(Island*) * MAX_ISLANDS);
    srand(time(NULL));
}

// Generate random float between min and max
static float randomFloatMan(float min, float max) {
    return min + (max - min) * ((float)rand() / RAND_MAX);
}

void regenerateIslands(IslandManager* manager) {
    // First properly free all existing islands
    for (int i = 0; i < manager->count; i++) {
        if (manager->islands[i]) {
            freeIslandResources(manager->islands[i]);
            free(manager->islands[i]);
            manager->islands[i] = NULL;
        }
    }
    manager->count = 0;

    float newRadius = randomFloatMan(ISLAND_MIN_RADIUS, ISLAND_MAX_RADIUS);

    // Create new islands at random positions with minimum distance
    for (int i = 0; i < 3; i++) {
        float x, z;
        bool validPosition;
        int attempts = 0;

        do {
            validPosition = true;
            x = (rand() % 60) - 30.0f;
            z = (rand() % 60) - 30.0f;

            // Check minimum distance from other islands
            for (int j = 0; j < manager->count; j++) {
                float dx = x - manager->islands[j]->position.x;
                float dz = z - manager->islands[j]->position.z;
                float distSq = dx * dx + dz * dz;
                if (distSq < (newRadius * 4) * (newRadius * 4)) {
                    validPosition = false;
                    break;
                }
            }

            attempts++;
            if (attempts > 100) {
                x = 0;
                z = 0;
                break; // Give up after too many attempts
            }
        } while (!validPosition);

        createIsland(manager, x, z);
    }
}

Island* createIsland(IslandManager* manager, float x, float z) {
    if (manager->count >= MAX_ISLANDS) return NULL;

    Island* island = (Island*)calloc(1, sizeof(Island));
    if (!island) return NULL;

    float randRadius = randomFloatMan(ISLAND_MIN_RADIUS, ISLAND_MAX_RADIUS);

    island->position.x = x;
    island->position.y = -2.0f;
    island->position.z = z;
    island->radius = randRadius;

    initIsland(island, randRadius);

    manager->islands[manager->count++] = island;
    return island;
}

void drawAllIslands(IslandManager* manager) {
    if (!manager) return;

    for (int i = 0; i < manager->count; i++) {
        if (manager->islands[i] && manager->islands[i]->isInitialized) {
            drawIsland(manager->islands[i]);
        }
    }
}

bool checkAllIslandsCollision(IslandManager* manager, Vec3 position, float radius) {
    if (!manager) return false;

    for (int i = 0; i < manager->count; i++) {
        if (manager->islands[i] &&
            checkIslandCollision(manager->islands[i], position, radius)) {
            return true;
        }
    }
    return false;
}

void drawIslandHitArea(IslandManager* manager, Vec3 playerPos, float playerRadius) {
    if (!manager) return;

    for (int i = 0; i < manager->count; i++) {
        drawNearestTriangleToPlayer(manager->islands[i], playerPos, playerRadius);
    }
}

void drawIndicator(Vec3 position) {
    float yOffset = 0.5f;         // Height above the position
    float size = 0.5f;            // Size of the triangle

    // Triangle will float above the position
    float x = position.x;
    float y = position.y + yOffset;
    float z = position.z;

    // Define 3 vertices of a triangle centered above the point
    // These form a flat triangle pointing up in the XZ plane
    GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 3);

    // Top vertex
    GX_Position3f32(x, y + size, z);
    GX_Color3f32(1.0f, 0.0f, 0.0f);

    // Bottom-left vertex
    GX_Position3f32(x - size, y, z);
    GX_Color3f32(0.0f, 1.0f, 0.0f);

    // Bottom-right vertex
    GX_Position3f32(x + size, y, z);
    GX_Color3f32(0.0f, 0.0f, 1.0f);

    GX_End();
}

void freeAllIslands(IslandManager* manager) {
    for (int i = 0; i < manager->count; i++) {
        if (manager->islands[i]) {
            freeIslandResources(manager->islands[i]);
            free(manager->islands[i]);
            manager->islands[i] = NULL;
        }
    }
    manager->count = 0;
}