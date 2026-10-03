// common.h
//
// Shared constants, types, and tiny math helpers used everywhere else.
// This is the single source of truth for tuning numbers -- change a
// value here rather than hunting through the .c files.

#pragma once
#include <stdbool.h>
#include <gccore.h>

// ============================================================
// World / wave constants
// ============================================================

// The Radius of the world, 80 by default for 160x160 patch
#define WORLD_RADIUS 80.0f

#define WAVE_FREQUENCY  0.5f   // Spatial frequency of the ocean wave (radians / world-unit)
#define WAVE_AMPLITUDE  0.15f  // Peak height of waves in world-units
#define WAVE_SPEED      0.03f  // How fast the wave phase advances each frame

// Y height at which the player is considered "at sea level" and can board
#define boatChangeY     0.5f

// Absolute floor -- the player never falls below this when there's no
// island underneath them.
#define BASE_Y         -1.0f

// The Y of every island's flat base circle. An island is a hemisphere
// (see island.h) -- imagine a dome-shaped bowl turned upside down and
// dropped in the ocean. The flat, open bottom face sits at this height,
// which is BELOW the water line, so that face is always hidden and the
// dome just looks like it's poking naturally out of the sea.
#define ISLAND_BASE_Y  -1.0f

// ============================================================
// Island generation tuning (Uniform Defaults)
// ============================================================

#define MAX_ISLANDS         5    // Max islands alive at once
#define numIslands          5    // Islands to seed at the start / around the player

// Uniform island qualities for now (before debug menu implementation)
#define ISLAND_DEFAULT_RADIUS       15.0f
#define ISLAND_DEFAULT_HEIGHT_SCALE  0.5f
#define ISLAND_DEFAULT_STYLE         0   // ISLAND_TROPICAL

#define NUM_ISLAND_STYLES    3    // Number of colour themes (TROPICAL, VOLCANO, ARCTIC)

// How finely we slice the dome for drawing. More segments = smoother
// looking dome but more triangles to push through the GPU every frame.
#define ISLAND_LON_SEGMENTS  16   // Slices going around the dome (like longitude)
#define ISLAND_LAT_SEGMENTS   8   // Bands going from base to peak (like latitude)

// ============================================================
// Streaming world constants
// ============================================================

// When the player moves this far from the last streaming update, generate
// a new ring of islands ahead of them and discard the farthest-away ones.
#define WORLD_STREAM_DISTANCE    80.0f
// Islands farther than this from the player are evicted.
#define WORLD_CULL_DISTANCE     160.0f
// Minimum gap between any two island centres (must be > 2*ISLAND_DEFAULT_RADIUS
// so two islands never overlap).
#define ISLAND_MIN_SEPARATION    55.0f

// ============================================================
// Player movement constants
// ============================================================

#define JUMP_FORCE  0.42f  // Upward velocity applied on jump

#define PLAYER_SNAP false  // Should player automatically turn to boat. boat to player on boarder?


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