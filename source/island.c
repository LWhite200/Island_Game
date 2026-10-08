// island.c
//
// Islands as triangle meshes with a KD-tree each, plus the manager that
// spawns / draws / queries them.

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <gccore.h>
#include "island.h"

// ============================================================
// SECTION: mesh generation
// ============================================================

// Island profile.
//
// The top is the original hemisphere (radius * heightScale tall), unchanged.
// A hemisphere is vertical at its rim, which is too steep to walk, so the
// dome is cut off where its slope has dropped to ISLAND_WALK_SLOPE and a
// gentle "beach" skirt continues outward from there:
//
//   - the skirt starts with exactly the dome's slope, so there is no crease
//   - it flattens smoothly to zero slope at the waterline
//
// The beach makes the island wider than `radius`, so by default the whole
// island is scaled down to fit back inside `radius` (same shape, same slopes,
// islands keep their spacing).  Define ISLAND_FIT_FOOTPRINT as 0 to keep the
// full-size dome and let the beach spill outward instead.
// The player treats surfaces steeper than about 1.0 (normal.y < 0.7) as
// walls, so keep this under ~0.9 to be able to walk the whole island.
// Lower = gentler (and wider) beach.
#ifndef ISLAND_FIT_FOOTPRINT
#define ISLAND_FIT_FOOTPRINT 1
#endif
#ifndef ISLAND_WALK_SLOPE
#define ISLAND_WALK_SLOPE 0.75f
#endif

// ============================================================
// SECTION: per-island variation
// ============================================================
//
// All of this is scaled by ISLAND_RANDOMNESS (0..1). At 0 every amplitude
// is exactly 0 and every multiplier exactly 1, so the generated mesh is
// bit-for-bit the plain dome.  At 1 you get the maximums below.
// Tweak the maximums to taste.

#define VAR_SIZE_MAX     1.0f   // radius        : +/- 25%
#define VAR_HEIGHT_MAX   2.5f   // height scale  : +/- 50%
#define VAR_TINT_BRIGHT  1.0f   // colour        : overall brightness +/- 20%
#define VAR_TINT_CHAN    1.0f   // colour        : extra per-channel shift +/- 10%

// Shape harmonics: frequency around the island, and max amplitude.
static const float kLumpFreq[ISLAND_LUMPS] = { 2.0f, 3.0f, 5.0f };  // oval, triangle-ish, lumpy
static const float kLumpMax [ISLAND_LUMPS] = { 0.20f, 0.10f, 0.06f };

// Height harmonics (lopsided / ridged tops).
static const float kBumpFreq[ISLAND_BUMPS] = { 2.0f, 3.0f };
static const float kBumpMax [ISLAND_BUMPS] = { 0.25f, 0.15f };

static float rand01(void)   { return (float)((rand() >> 8) & 0xFFFF) / 65536.0f; }  // [0, 1)
static float randSigned(void) { return rand01() * 2.0f - 1.0f; }                    // [-1, 1)

// Ring-radius multiplier at angle theta (1.0 for a perfect circle).
static float radialFactor(const Island* isle, float theta) {
    float f = 1.0f;
    for (int k = 0; k < ISLAND_LUMPS; k++)
        f += isle->lumpAmp[k] * cosf(kLumpFreq[k] * theta + isle->lumpPhase[k]);
    return f;
}

// Height multiplier bump at angle theta (0.0 for a symmetric dome).
static float heightBump(const Island* isle, float theta) {
    float f = 0.0f;
    for (int k = 0; k < ISLAND_BUMPS; k++)
        f += isle->bumpAmp[k] * cosf(kBumpFreq[k] * theta + isle->bumpPhase[k]);
    return f;
}

// Rolls this island's random parameters. Called once from createIsland().
static void rollIslandVariation(Island* isle, float rnd) {
    // Everything starts at "no variation".
    for (int k = 0; k < ISLAND_LUMPS; k++) { isle->lumpAmp[k] = 0.0f; isle->lumpPhase[k] = 0.0f; }
    for (int k = 0; k < ISLAND_BUMPS; k++) { isle->bumpAmp[k] = 0.0f; isle->bumpPhase[k] = 0.0f; }
    isle->tintR = isle->tintG = isle->tintB = 1.0f;

    if (rnd <= 0.0f) return;

    // Size and height.
    isle->radius      *= 1.0f + VAR_SIZE_MAX   * rnd * randSigned();
    isle->heightScale *= 1.0f + VAR_HEIGHT_MAX * rnd * randSigned();

    // Shape and height harmonics: random amplitude up to the max, random phase.
    for (int k = 0; k < ISLAND_LUMPS; k++) {
        isle->lumpAmp[k]   = kLumpMax[k] * rnd * rand01();
        isle->lumpPhase[k] = rand01() * 2.0f * (float)M_PI;
    }
    for (int k = 0; k < ISLAND_BUMPS; k++) {
        isle->bumpAmp[k]   = kBumpMax[k] * rnd * rand01();
        isle->bumpPhase[k] = rand01() * 2.0f * (float)M_PI;
    }

    // Colour: sometimes a different palette entirely, always a slight tint.
    if (rand01() < rnd)
        isle->colorStyle = (IslandColorStyle)(rand() % NUM_ISLAND_STYLES);

    float bright = VAR_TINT_BRIGHT * rnd * randSigned();
    isle->tintR = 1.0f + bright + VAR_TINT_CHAN * rnd * randSigned();
    isle->tintG = 1.0f + bright + VAR_TINT_CHAN * rnd * randSigned();
    isle->tintB = 1.0f + bright + VAR_TINT_CHAN * rnd * randSigned();
}

static bool buildIslandMesh(Island* isle) {
    const int LON = ISLAND_LON_SEGMENTS;
    const int LAT = ISLAND_LAT_SEGMENTS;

    isle->vertCount = LON * (LAT + 1);
    isle->verts = (Vec3*)malloc((size_t)isle->vertCount * sizeof(Vec3));
    if (!isle->verts) return false;

    // ---- Profile setup ----
    const float R  = isle->radius;
    const float Hp = R * isle->heightScale;          // peak height
    const float S  = ISLAND_WALK_SLOPE;

    // Dome slope at angle phi is heightScale / tan(phi); find where it hits S.
    const float phiC = atanf(isle->heightScale / S);
    const float rC   = R  * cosf(phiC);              // dome radius at the join
    const float hC   = Hp * sinf(phiC);              // dome height at the join
    float L = 2.0f * hC / S;                         // skirt length (flat at the end)
    if (L < 1e-4f) L = 1e-4f;

    // Uniform scale (applies to width and height, so slopes are unchanged).
#if ISLAND_FIT_FOOTPRINT
    const float fit = R / (rC + L);
#else
    const float fit = 1.0f;
#endif

    // Rings: the outer third are the skirt, the rest are the dome.
    int nSkirt = LAT / 3;
    if (nSkirt < 2)       nSkirt = 2;
    if (nSkirt > LAT - 1) nSkirt = LAT - 1;
    const int nDome = LAT - nSkirt;

    // ---- 1. Fill the vertex grid (j = 0 is the rim, j = LAT the peak) ----
    for (int j = 0; j <= LAT; j++) {
        float ring, h;

        if (j <= nSkirt) {
            // Skirt: x runs from L (waterline) back to 0 (join with the dome).
            float x = L * (1.0f - (float)j / (float)nSkirt);
            ring = rC + x;
            h    = hC - S * x + (S * x * x) / (2.0f * L);
        } else {
            // Dome: the original hemisphere from the join angle up to the top.
            float phi = phiC + ((float)M_PI * 0.5f - phiC)
                             * (float)(j - nSkirt) / (float)nDome;
            ring = R  * cosf(phi);
            h    = Hp * sinf(phi);
        }

        ring *= fit;
        h    *= fit;

        // Height variation fades to nothing at the peak (j == LAT) so the
        // top ring stays a single point instead of tearing into spikes.
        // (The rim, j == 0, is already at h == 0.)
        const float bumpWeight = 1.0f - (float)j / (float)LAT;

        for (int i = 0; i < LON; i++) {
            float theta = (i * 2.0f * M_PI) / LON;
            float rr = ring * radialFactor(isle, theta);
            float hh = h    * (1.0f + heightBump(isle, theta) * bumpWeight);
            isle->verts[j * LON + i] = (Vec3){
                isle->center.x + rr * cosf(theta),
                isle->center.y + hh,
                isle->center.z + rr * sinf(theta)
            };
        }
    }

    // ---- 2. Split every grid quad into 2 triangles for the KD-tree ----
    int triCount = LON * LAT * 2;
    Vec3* tris = (Vec3*)malloc((size_t)triCount * 3 * sizeof(Vec3));
    if (!tris) { free(isle->verts); isle->verts = NULL; return false; }

    int n = 0;
    for (int j = 0; j < LAT; j++) {
        for (int i = 0; i < LON; i++) {
            int i2 = (i + 1) % LON;                    // wrap around the island
            Vec3 p1 = isle->verts[ j      * LON + i ];
            Vec3 p2 = isle->verts[ j      * LON + i2];
            Vec3 p3 = isle->verts[(j + 1) * LON + i2];
            Vec3 p4 = isle->verts[(j + 1) * LON + i ];

            tris[n++] = p1; tris[n++] = p2; tris[n++] = p3;   // triangle 1
            tris[n++] = p1; tris[n++] = p3; tris[n++] = p4;   // triangle 2
        }
    }

    // ---- 3. Build the KD-tree ----
    bool ok = kdtreeBuild(&isle->tree, tris, triCount);
    free(tris);
    if (!ok) { free(isle->verts); isle->verts = NULL; return false; }
    return true;
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

    Island* isle = &manager->islands[manager->count];
    memset(isle, 0, sizeof(*isle));
    isle->center        = (Vec3){ x, ISLAND_BASE_Y, z };
    isle->radius        = ISLAND_DEFAULT_RADIUS;
    isle->heightScale   = ISLAND_DEFAULT_HEIGHT_SCALE;
    isle->colorStyle    = ISLAND_DEFAULT_STYLE;

    // Variation: 0 = clone of the defaults, 1 = maximum difference.
    float rnd = ISLAND_RANDOMNESS;
    if (rnd < 0.0f) rnd = 0.0f;
    if (rnd > 1.0f) rnd = 1.0f;
    rollIslandVariation(isle, rnd);

    if (!buildIslandMesh(isle)) return NULL;

    manager->count++;
    return isle;
}

void freeAllIslands(IslandManager* manager) {
    for (int i = 0; i < manager->count; i++) {
        free(manager->islands[i].verts);
        manager->islands[i].verts = NULL;
        kdtreeFree(&manager->islands[i].tree);
    }
    manager->count = 0;
}

// ============================================================
// SECTION: full regeneration (startup / debug reset)
// ============================================================

void regenerateIslands(IslandManager* manager) {
    freeAllIslands(manager);

    // 1. Create the central island at (0, 0)
    createIsland(manager, 0.0f, 0.0f);

    // 2. Create 4 surrounding islands in a symmetric ring
    float orbitDistance = 75.0f;
    for (int i = 0; i < 4; i++) {
        float angle = (i * 2.0f * M_PI) / 4.0f;
        float x = cosf(angle) * orbitDistance;
        float z = sinf(angle) * orbitDistance;
        createIsland(manager, x, z);
    }

    manager->lastStreamPos = (Vec3){0,0,0};
    manager->streamingInit = true;
}

// ============================================================
// SECTION: island colour (height-based gradient)
// ============================================================

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
// SECTION: drawing
// ============================================================

static void getVertexColor(int i, int j, float* r, float* g, float* b) {
    int choice = (i + j) % 3;
    if (choice == 0)      { *r = 1.0f; *g = 0.0f; *b = 0.0f; } // Red
    else if (choice == 1) { *r = 0.0f; *g = 1.0f; *b = 0.0f; } // Green
    else                  { *r = 0.0f; *g = 0.0f; *b = 1.0f; } // Blue
}

static float clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }

static void drawIsland(const Island* isle, Vec3 playerPos, float touchRadius) {
    const int LON = ISLAND_LON_SEGMENTS;
    const int LAT = ISLAND_LAT_SEGMENTS;
    float touchRadiusSq = touchRadius * touchRadius;

    for (int j = 0; j < LAT; j++) {
        float f1 = (float)j       / LAT;
        float f2 = (float)(j + 1) / LAT;
        float r1, g1, b1, r2, g2, b2;
        colorForHeightFraction(isle->colorStyle, f1, &r1, &g1, &b1);
        colorForHeightFraction(isle->colorStyle, f2, &r2, &g2, &b2);

        // Per-island tint (all 1.0 when randomness is 0).
        r1 = clamp01(r1 * isle->tintR); g1 = clamp01(g1 * isle->tintG); b1 = clamp01(b1 * isle->tintB);
        r2 = clamp01(r2 * isle->tintR); g2 = clamp01(g2 * isle->tintG); b2 = clamp01(b2 * isle->tintB);

        for (int i = 0; i < LON; i++) {
            int i2 = (i + 1) % LON;
            Vec3 p1 = isle->verts[ j      * LON + i  ];
            Vec3 p2 = isle->verts[ j      * LON + i2 ];
            Vec3 p3 = isle->verts[(j + 1) * LON + i2 ];
            Vec3 p4 = isle->verts[(j + 1) * LON + i  ];

            float vr1 = r1, vg1 = g1, vb1 = b1;
            float vr2 = r1, vg2 = g1, vb2 = b1;
            float vr3 = r2, vg3 = g2, vb3 = b2;
            float vr4 = r2, vg4 = g2, vb4 = b2;

            float d1_sq = (p1.x - playerPos.x)*(p1.x - playerPos.x) + (p1.y - playerPos.y)*(p1.y - playerPos.y) + (p1.z - playerPos.z)*(p1.z - playerPos.z);
            float d2_sq = (p2.x - playerPos.x)*(p2.x - playerPos.x) + (p2.y - playerPos.y)*(p2.y - playerPos.y) + (p2.z - playerPos.z)*(p2.z - playerPos.z);
            float d3_sq = (p3.x - playerPos.x)*(p3.x - playerPos.x) + (p3.y - playerPos.y)*(p3.y - playerPos.y) + (p3.z - playerPos.z)*(p3.z - playerPos.z);
            float d4_sq = (p4.x - playerPos.x)*(p4.x - playerPos.x) + (p4.y - playerPos.y)*(p4.y - playerPos.y) + (p4.z - playerPos.z)*(p4.z - playerPos.z);

            if (d1_sq <= touchRadiusSq) getVertexColor(i,  j,     &vr1, &vg1, &vb1);
            if (d2_sq <= touchRadiusSq) getVertexColor(i2, j,     &vr2, &vg2, &vb2);
            if (d3_sq <= touchRadiusSq) getVertexColor(i2, j + 1, &vr3, &vg3, &vb3);
            if (d4_sq <= touchRadiusSq) getVertexColor(i,  j + 1, &vr4, &vg4, &vb4);

            GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
            GX_Position3f32(p1.x, p1.y, p1.z); GX_Color3f32(vr1, vg1, vb1);
            GX_Position3f32(p2.x, p2.y, p2.z); GX_Color3f32(vr2, vg2, vb2);
            GX_Position3f32(p3.x, p3.y, p3.z); GX_Color3f32(vr3, vg3, vb3);
            GX_Position3f32(p4.x, p4.y, p4.z); GX_Color3f32(vr4, vg4, vb4);
            GX_End();
        }
    }
}

void drawAllIslands(IslandManager* manager, Vec3 playerPos, float touchRadius) {
    for (int i = 0; i < manager->count; i++)
        drawIsland(&manager->islands[i], playerPos, touchRadius);
}

// ============================================================
// SECTION: collision -- boat vs islands (sphere overlap)
// ============================================================

bool checkAllIslandsCollision(IslandManager* manager, Vec3 position, float radius) {
    for (int i = 0; i < manager->count; i++)
        if (kdtreeSphereOverlap(&manager->islands[i].tree, position, radius))
            return true;
    return false;
}

// ============================================================
// SECTION: wall push-out -- player vs steep triangles
// ============================================================

bool islandWallPush(IslandManager* manager, Vec3 center, float radius,
                    float maxNormalY, Vec3* outPush)
{
    bool  any = false;
    float bestLenSq = 0.0f;

    for (int i = 0; i < manager->count; i++) {
        Vec3 push;
        if (kdtreeSpherePush(&manager->islands[i].tree, center, radius, maxNormalY, &push)) {
            float lenSq = push.x*push.x + push.y*push.y + push.z*push.z;
            if (!any || lenSq > bestLenSq) {
                bestLenSq = lenSq;
                if (outPush) *outPush = push;
                any = true;
            }
        }
    }
    return any;
}

// ============================================================
// SECTION: ground height -- ray cast straight down
// ============================================================

float islandGroundHeight(IslandManager* manager, Vec3 position, float radius) {
    (void)radius;
    float best = ISLAND_NO_GROUND;
    const Vec3 down = { 0.0f, -1.0f, 0.0f };

    for (int i = 0; i < manager->count; i++) {
        KDHit hit;
        if (kdtreeRaycast(&manager->islands[i].tree, position, down, 1000.0f, &hit)) {
            if (hit.point.y > best) best = hit.point.y;
        }
    }
    return best;
}

// ============================================================
// SECTION: camera occlusion -- ray cast camera -> player
// ============================================================

bool checkCameraPlayerCovered(Vec3 cameraPos, Vec3 playerPos, IslandManager* manager) {
    Vec3 target = { playerPos.x, playerPos.y + 0.5f, playerPos.z };
    Vec3 dir    = { target.x-cameraPos.x, target.y-cameraPos.y, target.z-cameraPos.z };
    float dist  = sqrtf(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);
    if (dist < 1e-6f) return false;

    float maxDist = dist - 0.1f;
    if (maxDist <= 0.0f) return false;

    for (int i = 0; i < manager->count; i++)
        if (kdtreeRaycast(&manager->islands[i].tree, cameraPos, dir, maxDist, NULL))
            return true;
    return false;
}

// ============================================================
// SECTION: indicator triangle (boarding / landing prompt)
// ============================================================

void drawIndicator(Vec3 position) {
    const float Y_OFF = 0.6f, SZ = 0.4f;
    float x = position.x, y = position.y + Y_OFF, z = position.z;

    GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 3);
    GX_Position3f32(x,      y + SZ, z);  GX_Color3f32(1.0f, 0.0f, 0.0f);
    GX_Position3f32(x - SZ, y,      z);  GX_Color3f32(0.0f, 1.0f, 0.0f);
    GX_Position3f32(x + SZ, y,      z);  GX_Color3f32(0.0f, 0.0f, 1.0f);
    GX_End();
}