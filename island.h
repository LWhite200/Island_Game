#ifndef ISLAND_H
#define ISLAND_H

#include "common.h"
#include "kd_tree.h"

#define NUM_CTRL_POINTS 12
#define NUM_SEGMENTS 32

typedef enum {
    ISLAND_TROPICAL,
    ISLAND_VOLCANO,
    ISLAND_ARCTIC
} IslandType;

typedef struct {
    Vec3 position;
    float radius;
    bool isInitialized;
    IslandType colorStyle;
    KDNode* kdTree;
    void* vertices;  // Opaque pointer to vertex data
    int numVertices;
    float ctrlRadius[NUM_CTRL_POINTS];
    float ctrlHeight[NUM_CTRL_POINTS];
} Island;

void initIsland(Island* island, float baseRadius);
void drawIsland(Island* island);
bool checkIslandCollision(Island* island, Vec3 position, float radius);
void freeIslandResources(Island* island);
void drawNearestTriangleToPlayer(Island* island, Vec3 playerPos, float playerRadius);
bool cameraCoveredCheck(Vec3 cameraPos, Vec3 playerPos, Island* island);

#endif