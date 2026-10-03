// common.h
//
// Shared constants, types, and tiny math helpers used everywhere else.
// This is the single source of truth for tuning numbers.
//
// Most "constants" below are now real variables (defined in common.c)
// so the debug menu can change them at runtime. The old macro names
// are kept, so the rest of the code does not need to change:
//
//     WORLD_RADIUS  ->  (g_worldRadius)
//
// To change a DEFAULT value, edit it in common.c.
//
// MAX_ISLANDS and NUM_ISLAND_STYLES stay compile-time constants,
// because they size arrays / match the enum below.

#pragma once
#include <stdbool.h>
#include <gccore.h>

// ============================================================
// Compile-time constants (NOT editable in the debug menu)
// ============================================================

#define MAX_ISLANDS         5    // Max islands alive at once
#define NUM_ISLAND_STYLES   3    // Number of colour themes (TROPICAL, VOLCANO, ARCTIC)

// ============================================================
// Runtime-tunable values (defined in common.c)
// ============================================================

// World / wave
extern float g_worldRadius;
extern float g_waveFrequency;
extern float g_waveAmplitude;
extern float g_waveSpeed;
extern float g_boatChangeY;
extern float g_baseY;
extern float g_islandBaseY;

// Island generation
extern int   g_numIslands;
extern float g_islandDefaultRadius;
extern float g_islandDefaultHeightScale;
extern int   g_islandDefaultStyle;
extern int   g_islandLonSegments;
extern int   g_islandLatSegments;

// Streaming world
extern float g_worldStreamDistance;
extern float g_worldCullDistance;
extern float g_islandMinSeparation;

// Player
extern float g_jumpForce;
extern bool  g_playerSnap;

// ============================================================
// World / wave constants
// ============================================================

// The Radius of the world, 80 by default for 160x160 patch
#define WORLD_RADIUS    (g_worldRadius)

#define WAVE_FREQUENCY  (g_waveFrequency)   // Spatial frequency of the ocean wave (radians / world-unit)
#define WAVE_AMPLITUDE  (g_waveAmplitude)   // Peak height of waves in world-units
#define WAVE_SPEED      (g_waveSpeed)       // How fast the wave phase advances each frame

// Y height at which the player is considered "at sea level" and can board
#define boatChangeY     (g_boatChangeY)

// Absolute floor -- the player never falls below this when there's no
// island underneath them.
#define BASE_Y          (g_baseY)

// The Y of every island's flat base circle. An island is a hemisphere
// (see island.h) -- imagine a dome-shaped bowl turned upside down and
// dropped in the ocean. The flat, open bottom face sits at this height,
// which is BELOW the water line, so that face is always hidden and the
// dome just looks like it's poking naturally out of the sea.
#define ISLAND_BASE_Y   (g_islandBaseY)

// ============================================================
// Island generation tuning (Uniform Defaults)
// ============================================================

#define numIslands                  (g_numIslands)   // Islands to seed at the start / around the player (max MAX_ISLANDS)

// Uniform island qualities for now
#define ISLAND_DEFAULT_RADIUS       (g_islandDefaultRadius)
#define ISLAND_DEFAULT_HEIGHT_SCALE (g_islandDefaultHeightScale)
#define ISLAND_DEFAULT_STYLE        (g_islandDefaultStyle)   // 0 = ISLAND_TROPICAL

// How finely we slice the dome for drawing. More segments = smoother
// looking dome but more triangles to push through the GPU every frame.
#define ISLAND_LON_SEGMENTS         (g_islandLonSegments)   // Slices going around the dome (like longitude)
#define ISLAND_LAT_SEGMENTS         (g_islandLatSegments)   // Bands going from base to peak (like latitude)

// ============================================================
// Streaming world constants
// ============================================================

// When the player moves this far from the last streaming update, generate
// a new ring of islands ahead of them and discard the farthest-away ones.
#define WORLD_STREAM_DISTANCE       (g_worldStreamDistance)
// Islands farther than this from the player are evicted.
#define WORLD_CULL_DISTANCE         (g_worldCullDistance)
// Minimum gap between any two island centres (must be > 2*ISLAND_DEFAULT_RADIUS
// so two islands never overlap).
#define ISLAND_MIN_SEPARATION       (g_islandMinSeparation)

// ============================================================
// Player movement constants
// ============================================================

#define JUMP_FORCE      (g_jumpForce)   // Upward velocity applied on jump

#define PLAYER_SNAP     (g_playerSnap)  // Should player automatically turn to boat. boat to player on boarder?


// ============================================================
// Vec3 -- our lightweight 3-D vector, used everywhere.
// ============================================================

typedef struct { float x, y, z; } Vec3;

// ============================================================
// Island colour styles -- purely cosmetic, picked at random per island.
// ============================================================

typedef enum {
    ISLAND_TROPICAL = 0,
    ISLAND_VOLCANO  = 1,
    ISLAND_ARCTIC   = 2,
} IslandColorStyle;

// ============================================================
// Simple inline math helpers (shared everywhere)
// ============================================================

static inline float vec3DistSq(Vec3 a, Vec3 b) {
    float dx = a.x-b.x, dy = a.y-b.y, dz = a.z-b.z;
    return dx*dx + dy*dy + dz*dz;
}

static inline float vec3Dist(Vec3 a, Vec3 b) {
    float d2 = vec3DistSq(a, b);
    return (d2 < 1e-12f) ? 0.0f : __builtin_sqrtf(d2);
}