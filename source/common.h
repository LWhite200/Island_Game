// common.h
//
// Shared constants, types, and tiny math helpers used everywhere else.
// This is the single source of truth for tuning numbers -- change a
// value here rather than hunting through the .c files.
//
// NOTE: everything to do with enemies ("bodies") and collectibles has
// been removed from the game, so there's nothing about them in here
// anymore either.

#pragma once
#include <stdbool.h>
#include <gccore.h>

// ============================================================
// World / wave constants
// ============================================================

#define WAVE_FREQUENCY  0.5f   // Spatial frequency of the ocean wave (radians / world-unit)
#define WAVE_AMPLITUDE  0.15f  // Peak height of waves in world-units
#define WAVE_SPEED      0.03f  // How fast the wave phase advances each frame

// Y height at which the player is considered "at sea level" and can board
#define boatChangeY     0.5f

// Absolute floor -- the player never falls below this when there's no
// island underneath them.
#define BASE_Y         -2.0f

// The Y of every island's flat base circle. An island is a hemisphere
// (see island.h) -- imagine a dome-shaped bowl turned upside down and
// dropped in the ocean. The flat, open bottom face sits at this height,
// which is BELOW the water line, so that face is always hidden and the
// dome just looks like it's poking naturally out of the sea.
#define ISLAND_BASE_Y  -3.5f

// ============================================================
// Island generation tuning
// ============================================================

#define MAX_ISLANDS         32    // Max islands alive at once
#define numIslands           6    // Islands to seed at the start / around the player

// A hemisphere's height above its base is always equal to its radius,
// so radius alone controls both how wide AND how tall an island is.
#define ISLAND_MIN_RADIUS   10.0f
#define ISLAND_MAX_RADIUS   22.0f

// New tuning constant: Scales the height relative to the radius.
// 1.0f = standard hemisphere (height equals radius), 
// 0.5f = flattened hills, 2.0f = steep mountains.
#define ISLAND_MIN_HEIGHT_SCALE  0.3f
#define ISLAND_MAX_HEIGHT_SCALE  1.0f

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
#define WORLD_STREAM_DISTANCE   80.0f
// Islands farther than this from the player are evicted.
#define WORLD_CULL_DISTANCE    160.0f
// Minimum gap between any two island centres (must be > 2*ISLAND_MAX_RADIUS
// so two islands never overlap).
#define ISLAND_MIN_SEPARATION   55.0f

// ============================================================
// Ocean obstacles (plain rock pillars scattered in the sea)
// ============================================================

#define MAX_OBSTACLES   24
#define OBSTACLE_RADIUS  0.8f

// ============================================================
// Player movement constants
// ============================================================

#define JUMP_FORCE  0.28f  // Upward velocity applied on jump

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
// Ocean obstacle -- a static rock pillar in the sea
// ============================================================

typedef struct {
    Vec3  position;
    float radius;
    float height;       // How tall the pillar protrudes above water
} OceanObstacle;

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
