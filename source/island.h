// island.h
//
// An island is a triangle MESH that sits in the ocean. Right now the mesh is
// generated as a dome (a squashed hemisphere), but nothing outside island.c
// assumes that -- the mesh is the single source of truth for both drawing
// and collision. To make lumpier islands later, just change how the vertices
// are generated in buildIslandMesh() (island.c); collision keeps working.
//
// COLLISION
// ------------------------------------------------------------
// Each island owns a KD-tree (see kdtree.h) built once from its triangles.
// Every question we ask an island is answered by querying that tree:
//
//   "how high is the ground here?"      -> ray cast straight down
//   "is the boat touching land?"        -> sphere overlap
//   "is the player walking into a wall?"-> sphere push-out
//   "is an island blocking the camera?" -> ray cast from camera to player
//
// MEMORY
// ------------------------------------------------------------
// Meshes and trees live on the heap, so islands must be released with
// freeAllIslands() (it is called for you by regenerateIslands()).

#pragma once
#include "common.h"
#include "kdtree.h"
#include <stdbool.h>

// Returned by islandGroundHeight() when there is no island below a point.
#define ISLAND_NO_GROUND  (-1.0e9f)

// How many shape harmonics / height harmonics each island carries.
// The matching frequencies live in island.c (kLumpFreq / kBumpFreq).
#define ISLAND_LUMPS  3
#define ISLAND_BUMPS  2

// ---- A single island ----
typedef struct {
    // --- Description (what the generator was asked for) ---
    Vec3  center;        // Centre of the island's flat, underwater base circle
    float radius;        // Footprint radius
    float heightScale;   // Scales the dome height relative to the radius
    IslandColorStyle colorStyle;  // Picks a colour palette when drawing

    // --- Variation (rolled once in createIsland, scaled by ISLAND_RANDOMNESS) ---
    // With randomness 0 every amplitude is 0 and every tint is 1, so the
    // island is the plain round dome.
    //
    // Shape: the ring radius is multiplied by 1 + sum(lumpAmp[k] * cos(freq*theta + phase)).
    //   freq 2 stretches it into an oval, freq 3+ makes it lumpy.
    // Bumps: the same idea for height, fading out toward the peak (so the
    // peak ring stays a single point).
    // Tint: multiplies the palette colour.
    float lumpAmp[ISLAND_LUMPS];
    float lumpPhase[ISLAND_LUMPS];
    float bumpAmp[ISLAND_BUMPS];
    float bumpPhase[ISLAND_BUMPS];
    float tintR, tintG, tintB;

    // --- Mesh (what actually gets drawn) ---
    // A grid of vertices: ISLAND_LON_SEGMENTS columns (wrapping around the
    // island) by ISLAND_LAT_SEGMENTS+1 rows (rim at row 0, peak at the top).
    // Vertex (column i, row j) lives at verts[j * ISLAND_LON_SEGMENTS + i].
    Vec3* verts;
    int   vertCount;

    // --- Collision (built from the mesh) ---
    KDTree tree;
} Island;

// ---- The pool of all islands ----
typedef struct IslandManager {
    Island islands[MAX_ISLANDS];
    int    count;

    // World-streaming bookkeeping
    Vec3 lastStreamPos;   // Player/boat position at the last streaming event
    bool streamingInit;   // Becomes true after the first regenerateIslands()
} IslandManager;

// ---- Lifecycle ----
void initIslandManager (IslandManager* manager);
void regenerateIslands (IslandManager* manager); // Wipes and re-seeds everything
void freeAllIslands    (IslandManager* manager); // Frees every mesh + KD-tree

// Creates one island (mesh + KD-tree). Returns NULL if the pool is full or
// memory ran out.
Island* createIsland (IslandManager* manager, float x, float z);


// ---- Rendering ----
void drawAllIslands(IslandManager* manager, Vec3 playerPos, float touchRadius);
void drawIndicator    (Vec3 position); // Small floating triangle: "you can board/land here"

// ---- Collision / height queries (checked against ALL islands) ----

// True if a sphere at `position` touches any island.
bool  checkAllIslandsCollision (IslandManager* manager, Vec3 position, float radius);

// Casts a ray straight DOWN starting at `position` and returns the Y of the
// highest island surface it hits, or ISLAND_NO_GROUND if there is none.
// (Start the ray a little above the feet so small bumps are stepped over.)
float islandGroundHeight       (IslandManager* manager, Vec3 position, float radius);

// Pushes a sphere out of island WALLS (triangles steeper than maxNormalY --
// see kdtreeSpherePush). Returns true and the push vector if it was touching.
bool  islandWallPush           (IslandManager* manager, Vec3 center, float radius,
                                float maxNormalY, Vec3* outPush);

// True if any island lies between the camera and the player.
bool  checkCameraPlayerCovered (Vec3 cameraPos, Vec3 playerPos, IslandManager* manager);
