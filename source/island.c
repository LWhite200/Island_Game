// island.c
//
// Islands as hemispheres, plus the manager that spawns/streams/draws them.
//
// The key trick that makes this whole file simple: for a point at
// horizontal distance `d` from an island's centre, the dome's surface
// height is
//
//     y = center.y + sqrt(radius^2 - d^2)          (only valid for d <= radius)
//
// That's just the equation of a circle (Pythagoras), lifted into 3-D.
// Every collision / height / occlusion check below is built from that one
// idea, or from a plain sphere-distance check (since a hemisphere IS the
// top half of a sphere).

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <gccore.h>
#include "island.h"

// ============================================================
// SECTION: small helpers
// ============================================================

static float rndFloat(float min, float max) {
    return min + (max - min) * ((float)rand() / RAND_MAX);
}

// ============================================================
// SECTION: island creation
// ============================================================

void initIslandManager(IslandManager* manager) {
    manager->count         = 0;
    manager->streamingInit = false;
    manager->lastStreamPos = (Vec3){0,0,0};
    memset(manager->islands,   0, sizeof(manager->islands));
    srand((unsigned int)time(NULL));
}

Island* createIsland(IslandManager* manager, float x, float z) {
    if (manager->count >= MAX_ISLANDS) return NULL;

    Island* isle = &manager->islands[manager->count++];
    isle->center      = (Vec3){ x, ISLAND_BASE_Y, z };
    isle->radius      = rndFloat(ISLAND_MIN_RADIUS, ISLAND_MAX_RADIUS);
    isle->heightScale = rndFloat(ISLAND_MIN_HEIGHT_SCALE, ISLAND_MAX_HEIGHT_SCALE);
    isle->colorStyle  = rand() % NUM_ISLAND_STYLES;
    return isle;
}

void freeAllIslands(IslandManager* manager) {
    manager->count = 0;
}

// ============================================================
// SECTION: full regeneration (startup / debug reset)
// ============================================================

void regenerateIslands(IslandManager* manager) {
    freeAllIslands(manager);

    for (int i = 0; i < numIslands; i++) {
        float x, z;
        bool  ok    = false;
        int   tries = 0;
        do {
            ok = true;
            x  = rndFloat(-80.0f, 80.0f);
            z  = rndFloat(-80.0f, 80.0f);
            for (int j = 0; j < manager->count; j++) {
                float dx = x - manager->islands[j].center.x;
                float dz = z - manager->islands[j].center.z;
                if (dx*dx + dz*dz < ISLAND_MIN_SEPARATION * ISLAND_MIN_SEPARATION) {
                    ok = false; break;
                }
            }
        } while (!ok && ++tries < 100);
        createIsland(manager, x, z);
    }

    manager->lastStreamPos = (Vec3){0,0,0};
    manager->streamingInit = true;
}

// ============================================================
// SECTION: island colour (height-based gradient, purely cosmetic)
// ============================================================

// `f` is how far up the dome we are, from 0 (the waterline rim) to 1 (the peak).
static void colorForHeightFraction(IslandColorStyle style, float f, float* r, float* g, float* b) {
    switch (style) {
        case ISLAND_TROPICAL:
            if      (f < 0.10f) { *r=0.96f; *g=0.87f; *b=0.65f; } // sand
            else if (f < 0.35f) { *r=0.40f; *g=0.80f; *b=0.40f; } // grass
            else if (f < 0.65f) { *r=0.10f; *g=0.50f; *b=0.10f; } // jungle
            else if (f < 0.90f) { *r=0.30f; *g=0.30f; *b=0.20f; } // rock
            else                { *r=0.60f; *g=0.60f; *b=0.60f; } // bare stone peak
            break;
        case ISLAND_VOLCANO:
            if      (f < 0.10f) { *r=0.25f; *g=0.15f; *b=0.05f; } // ash
            else if (f < 0.35f) { *r=0.40f; *g=0.40f; *b=0.40f; } // charcoal
            else if (f < 0.65f) { *r=0.60f; *g=0.10f; *b=0.05f; } // molten red
            else if (f < 0.90f) { *r=0.30f; *g=0.05f; *b=0.05f; } // lava rock
            else                { *r=0.10f; *g=0.05f; *b=0.05f; } // scorched peak
            break;
        case ISLAND_ARCTIC:
        default:
            if      (f < 0.10f) { *r=0.85f; *g=0.92f; *b=1.00f; } // icy blue
            else if (f < 0.35f) { *r=0.75f; *g=0.85f; *b=0.95f; } // snowy
            else if (f < 0.65f) { *r=0.65f; *g=0.75f; *b=0.90f; } // frosty
            else if (f < 0.90f) { *r=0.50f; *g=0.60f; *b=0.75f; } // ice rock
            else                { *r=0.40f; *g=0.40f; *b=0.60f; } // arctic peak
            break;
    }
}

// ============================================================
// SECTION: drawing the dome
// ============================================================
// The dome is generated fresh every frame, straight from centre + radius --
// there's no stored mesh anywhere. That costs a bit of CPU (some sin/cos
// per vertex, per island, per frame) but means an island is *just* a
// centre and a radius, nothing else, which is the whole point here.
// If this ever becomes a performance problem, the fix is to build each
// island's vertex list once (like the original code did) and cache it.

static void drawIsland(Island* isle) {
    const int LON = ISLAND_LON_SEGMENTS;
    const int LAT = ISLAND_LAT_SEGMENTS;
    float R = isle->radius;

    for (int i = 0; i < LON; i++) {
        float theta1 = (i       * 2.0f * M_PI) / LON;
        float theta2 = ((i + 1) * 2.0f * M_PI) / LON;

        for (int j = 0; j < LAT; j++) {
            // phi = angle up from the equator (0 = waterline rim, PI/2 = peak)
            float phi1 = (j       * (M_PI / 2.0f)) / LAT;
            float phi2 = ((j + 1) * (M_PI / 2.0f)) / LAT;

            // Standard hemisphere parametrisation:
            //   horizontal radius at this band = R * cos(phi)
            //   height above the base           = R * sin(phi)
            // Height above the base now factors in the height scale multiplier
            float ring1 = R * cosf(phi1), h1 = R * sinf(phi1) * isle->heightScale;
            float ring2 = R * cosf(phi2), h2 = R * sinf(phi2) * isle->heightScale;

            Vec3 p1 = { isle->center.x + ring1*cosf(theta1), isle->center.y + h1, isle->center.z + ring1*sinf(theta1) };
            Vec3 p2 = { isle->center.x + ring1*cosf(theta2), isle->center.y + h1, isle->center.z + ring1*sinf(theta2) };
            Vec3 p3 = { isle->center.x + ring2*cosf(theta2), isle->center.y + h2, isle->center.z + ring2*sinf(theta2) };
            Vec3 p4 = { isle->center.x + ring2*cosf(theta1), isle->center.y + h2, isle->center.z + ring2*sinf(theta1) };

            float f1 = phi1 / (M_PI / 2.0f); // 0..1 fraction up the dome
            float f2 = phi2 / (M_PI / 2.0f);
            float r1,g1,b1, r2,g2,b2;
            colorForHeightFraction(isle->colorStyle, f1, &r1, &g1, &b1);
            colorForHeightFraction(isle->colorStyle, f2, &r2, &g2, &b2);

            GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
            GX_Position3f32(p1.x, p1.y, p1.z); GX_Color3f32(r1, g1, b1);
            GX_Position3f32(p2.x, p2.y, p2.z); GX_Color3f32(r1, g1, b1);
            GX_Position3f32(p3.x, p3.y, p3.z); GX_Color3f32(r2, g2, b2);
            GX_Position3f32(p4.x, p4.y, p4.z); GX_Color3f32(r2, g2, b2);
            GX_End();
        }
    }
}

void drawAllIslands(IslandManager* manager) {
    for (int i = 0; i < manager->count; i++)
        drawIsland(&manager->islands[i]);
}

// ============================================================
// SECTION: collision -- an island behaves like a plain sphere
// ============================================================
// A hemisphere IS the top half of a sphere, and nothing in this game ever
// goes below the flat base (that's underwater / below the world floor
// anyway) -- so testing against the *full* sphere gives exactly the same
// answer as testing against just the dome, with much simpler code.

static bool checkIslandCollision(Island* isle, Vec3 position, float radius) {
    float distSq  = vec3DistSq(position, isle->center);
    float touchDist = isle->radius + radius;
    return distSq <= touchDist * touchDist;
}

bool checkAllIslandsCollision(IslandManager* manager, Vec3 position, float radius) {
    for (int i = 0; i < manager->count; i++)
        if (checkIslandCollision(&manager->islands[i], position, radius))
            return true;
    return false;
}

// ============================================================
// SECTION: ground height -- straight from the dome formula
// ============================================================
// Given an XZ position, how high is the dome surface directly below it?
// If several islands overlap (they shouldn't, thanks to ISLAND_MIN_SEPARATION,
// but just in case) we stand on whichever gives the highest surface.

float islandGroundHeight(IslandManager* manager, Vec3 position, float radius) {
    (void)radius;
    float best  = position.y;
    bool  found = false;

    for (int i = 0; i < manager->count; i++) {
        Island* isle = &manager->islands[i];
        float dx = position.x - isle->center.x;
        float dz = position.z - isle->center.z;
        float d  = sqrtf(dx*dx + dz*dz);

        if (d <= isle->radius) {
            float baseH = sqrtf(isle->radius*isle->radius - d*d);
            float h = isle->center.y + (baseH * isle->heightScale);
            if (!found || h > best) { best = h; found = true; }
        }
    }
    return best;
}

// ============================================================
// SECTION: camera occlusion -- ray vs sphere
// ============================================================
// Is there island geometry between the camera and the player? A hemisphere
// is a sphere, so this is the classic, short ray-vs-sphere test.

static bool raySphereHit(Vec3 origin, Vec3 dir, float maxDist, Vec3 center, float radius) {
    Vec3  oc = { origin.x-center.x, origin.y-center.y, origin.z-center.z };
    float b  = oc.x*dir.x + oc.y*dir.y + oc.z*dir.z;   // dir must be normalised
    float c  = (oc.x*oc.x + oc.y*oc.y + oc.z*oc.z) - radius*radius;
    float disc = b*b - c;
    if (disc < 0.0f) return false;       // Ray misses the sphere entirely
    float t = -b - sqrtf(disc);          // Distance to the near intersection
    return (t > 0.0f && t < maxDist);    // Hit happens strictly between camera and player
}

bool checkCameraPlayerCovered(Vec3 cameraPos, Vec3 playerPos, IslandManager* manager) {
    Vec3  dir  = { playerPos.x-cameraPos.x, playerPos.y-cameraPos.y, playerPos.z-cameraPos.z };
    float dist = sqrtf(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);
    if (dist < 1e-6f) return false;
    dir.x /= dist; dir.y /= dist; dir.z /= dist;

    for (int i = 0; i < manager->count; i++) {
        Island* isle = &manager->islands[i];
        if (raySphereHit(cameraPos, dir, dist, isle->center, isle->radius))
            return true;
    }
    return false;
}

// ============================================================
// SECTION: indicator triangle (boarding / landing prompt)
// ============================================================

void drawIndicator(Vec3 position) {
    const float Y_OFF = 0.6f, SZ = 0.4f;
    float x = position.x, y = position.y + Y_OFF, z = position.z;

    GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 3);
    GX_Position3f32(x,      y + SZ, z);  GX_Color3f32(1.0f, 0.9f, 0.0f);
    GX_Position3f32(x - SZ, y,      z);  GX_Color3f32(1.0f, 0.6f, 0.0f);
    GX_Position3f32(x + SZ, y,      z);  GX_Color3f32(1.0f, 0.6f, 0.0f);
    GX_End();
}
