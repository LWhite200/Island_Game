#ifndef ISLAND_MANAGER_H
#define ISLAND_MANAGER_H

#include "island.h"
#include "common.h"

#define MAX_ISLANDS 10

typedef struct {
    Island* islands[MAX_ISLANDS];
    int count;
} IslandManager;

void initIslandManager(IslandManager* manager);
Island* createIsland(IslandManager* manager, float x, float z);
void drawAllIslands(IslandManager* manager);
bool checkAllIslandsCollision(IslandManager* manager, Vec3 position, float radius);
void freeAllIslands(IslandManager* manager);
void regenerateIslands(IslandManager* manager);  // Add this line
void drawIndicator(Vec3 position);
void drawIslandHitArea(IslandManager* manager, Vec3 playerPos, float playerRadius);
bool checkCameraPlayerCovered(Vec3 cameraPos, Vec3 playerPos, IslandManager* manager);

#endif