#include "manager.h"
#include <stdlib.h>
#include <time.h>
#include <string.h>

void initIslandManager(IslandManager* manager) {
    manager->count = 0;
    memset(manager->islands, 0, sizeof(Island*) * MAX_ISLANDS);
    srand(time(NULL));
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
                if (distSq < (ISLAND_RADIUS * 4) * (ISLAND_RADIUS * 4)) {
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

        createIsland(manager, x, z, ISLAND_RADIUS);
    }
}

Island* createIsland(IslandManager* manager, float x, float z, float radius) {
    if (manager->count >= MAX_ISLANDS) return NULL;

    Island* island = (Island*)calloc(1, sizeof(Island));
    if (!island) return NULL;

    island->position.x = x;
    island->position.y = 0.0f;
    island->position.z = z;
    island->radius = radius;

    initIsland(island, radius);

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