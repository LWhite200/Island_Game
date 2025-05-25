#ifndef ISLAND_H
#define ISLAND_H

#include "common.h"
#include "kd_tree.h"

#define NUM_CTRL_POINTS 12  // Increased for smoother shapes
#define NUM_SEGMENTS 32

typedef enum {
    ISLAND_TROPICAL,
    ISLAND_VOLCANO,
    ISLAND_ARCTIC
} IslandType;

void initIsland(float baseRadius);
void drawIsland();
bool checkIslandCollision(Vec3 position, float radius);
void freeIslandResources();

#endif