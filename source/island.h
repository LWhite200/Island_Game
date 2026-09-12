// island.h
//
// An island is just a hemisphere: picture a solid dome, like half a ball,
// sitting in the ocean. Its flat circular face (the "open bottom" of the
// hemisphere) points straight down and sits below the water line, so you
// never see it -- all you ever see is the curved dome breaking the surface.
//
// Because the shape is this simple, there is NO mesh stored anywhere and
// NO collision structure (no KD-tree, no triangles) -- every question we
// ever need to ask an island ("how tall are you here?", "am I touching
// you?", "are you blocking the camera?") has a one-line formula answer
// once you know its centre and radius. See island.c for those formulas.
//
// This file also owns the IslandManager, which is just "the pool of
// islands that currently exist" plus the ocean-rock obstacles and the
// logic that streams new islands in as the player explores.

#pragma once
#include "common.h"
#include <stdbool.h>

// ---- A single island ----
typedef struct {
    Vec3  center;   // Centre of the island's flat, underwater base circle
    float radius;   // Footprint radius. A hemisphere's height above its
                     // base always equals its radius, so this one number
                     // controls both how wide AND how tall the island is.

    float heightScale;   
    IslandColorStyle colorStyle;  // Just picks a colour palette when drawing
} Island;

// ---- The pool of all islands + ocean obstacles ----
typedef struct IslandManager {
    Island islands[MAX_ISLANDS];
    int    count;

    OceanObstacle obstacles[MAX_OBSTACLES];
    int           obstacleCount;

    // World-streaming bookkeeping
    Vec3 lastStreamPos;   // Player/boat position at the last streaming event
    bool streamingInit;   // Becomes true after the first regenerateIslands()
} IslandManager;

// ---- Lifecycle ----
void initIslandManager (IslandManager* manager);
void regenerateIslands (IslandManager* manager); // Wipes and re-seeds everything
void freeAllIslands    (IslandManager* manager); // Just resets counts to 0 --
                                                  // islands are plain data, so
                                                  // there is nothing to free()

Island* createIsland (IslandManager* manager, float x, float z);

// ---- World streaming ----
// Call once per frame with the current player/boat position. Spawns new
// islands ahead of travel and evicts ones left far behind.
void updateWorldStreaming (IslandManager* manager, Vec3 playerPos);

// ---- Obstacles ----
void generateObstacles (IslandManager* manager);

// ---- Rendering ----
void drawAllIslands   (IslandManager* manager);
void drawAllObstacles (IslandManager* manager, float time);
void drawIndicator    (Vec3 position); // Small floating triangle: "you can board/land here"

// ---- Collision / height queries (checked against ALL islands) ----
bool  checkAllIslandsCollision (IslandManager* manager, Vec3 position, float radius);
bool  checkObstacleCollision   (IslandManager* manager, Vec3 position, float radius);
float islandGroundHeight       (IslandManager* manager, Vec3 position, float radius);
bool  checkCameraPlayerCovered (Vec3 cameraPos, Vec3 playerPos, IslandManager* manager);
