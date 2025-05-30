#ifndef BODY_MANAGER_H
#define BODY_MANAGER_H

#include "body.h"
#include "common.h"
#include "manager.h"  // Only used for spawning and collision
#include <stdbool.h>

#define MAX_BODIES 64

typedef struct {
    Body bodies[MAX_BODIES];
    int count;
} BodyManager;

void initBodyManager(BodyManager* manager);
void spawnBodiesOnIslands(BodyManager* manager, IslandManager* islands);
void updateBodies(BodyManager* manager, IslandManager* islands, Vec3 playerPos);
void drawBodies(BodyManager* manager);

#endif
