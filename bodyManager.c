#include "bodyManager.h"
#include <stdlib.h>
#include <math.h>

// Helper: Random float in range
static float randomFloat(float min, float max) {
    return min + (max - min) * ((float)rand() / RAND_MAX);
}

void initBodyManager(BodyManager* manager) {
    manager->count = 0;
}

// Spawns 1–4 bodies randomly on each island
void spawnBodiesOnIslands(BodyManager* manager, IslandManager* islands) {
    for (int i = 0; i < islands->count; i++) {
        Island* island = islands->islands[i];
        if (!island) continue;

        int numBodies = rand() % 10 + 1;  // 1–4 per island

        for (int j = 0; j < numBodies && manager->count < MAX_BODIES; j++) {
            float angle = randomFloat(0, 2 * M_PI);
            float distance = randomFloat(0.0f, island->radius * 0.45f);  // Keep them within island
            float x = island->position.x + cosf(angle) * distance;
            float z = island->position.z + sinf(angle) * distance;
            float y = 20;  // Start at base level

            initBody(&manager->bodies[manager->count++], x, y, z);
        }
    }
}

void updateBodies(BodyManager* manager, IslandManager* islands, Vec3 playerPos) {
    for (int i = 0; i < manager->count; i++) {
        updateBody(&manager->bodies[i], islands, playerPos);
    }
}

void drawBodies(BodyManager* manager) {
    for (int i = 0; i < manager->count; i++) {
        drawBody(&manager->bodies[i]);
    }
}
