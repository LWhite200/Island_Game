// boat.c
// Player-controlled boat.
//
// The boat's Y is computed from the wave formula every frame so it visually
// bobs on the water surface. Horizontal movement is blocked by island terrain,
// ocean obstacles, and the world boundaries ($\pm$WORLD_RADIUS).

#include <gccore.h>
#include <math.h>
#include "boat.h"

// ============================================================
// Initialisation
// ============================================================

void initBoat(Boat* boat) {
    boat->position = (guVector){ 0.0f, 0.0f, 0.0f };
    boat->yaw      = 0.0f;
    boat->speed    = 0.2f;
    boat->radius   = 1.0f;
}

// ============================================================
// Per-frame update
// ============================================================

// Computes the wave height at a given XZ world position.
static float waveHeightAt(float x, float z, float time) {
    return sinf((x + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE
         + cosf((z + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE;
}

// Checks if a given position exceeds the world radius bounds considering the boat's radius.
static bool checkWorldBoundary(Vec3 pos, float radius) {
    return (pos.x < -WORLD_RADIUS + radius || pos.x > WORLD_RADIUS - radius ||
            pos.z < -WORLD_RADIUS + radius || pos.z > WORLD_RADIUS - radius);
}

bool updateBoat(Boat* boat,
                bool upp, bool down, bool left, bool right,
                float time, IslandManager* islandManager)
{
    bool pushedLand = false;

    // --- Live wave Y for this frame ---
    float waveY = waveHeightAt(boat->position.x, boat->position.z, time);

    // Update the stored Y to match the wave so callers can read boat->position.y
    boat->position.y = waveY;

    // --- Build candidate positions for collision ---
    Vec3 curPos = { boat->position.x, waveY, boat->position.z };

    Vec3 fwdPos = {
        boat->position.x - sinf(boat->yaw) * boat->speed,
        waveY,
        boat->position.z + cosf(boat->yaw) * boat->speed
    };
    Vec3 bwdPos = {
        boat->position.x + sinf(boat->yaw) * boat->speed,
        waveY,
        boat->position.z - cosf(boat->yaw) * boat->speed
    };

    // Check island terrain, ocean obstacles, AND world boundaries
    bool frontBlocked  = checkAllIslandsCollision(islandManager, fwdPos, boat->radius) || checkWorldBoundary(fwdPos, boat->radius);
    bool behindBlocked = checkAllIslandsCollision(islandManager, bwdPos, boat->radius) || checkWorldBoundary(bwdPos, boat->radius);
    bool curBlocked    = checkAllIslandsCollision(islandManager, curPos, boat->radius) || checkWorldBoundary(curPos, boat->radius);

    // Show collision/boarding indicator when the boat touches land or the world border
    if (frontBlocked || curBlocked || behindBlocked)
        drawIndicator(curPos);

    // --- Movement ---
    // Detect land separately from world-boundary collision.  A land hit is
    // reported to main.c so PLAYER_SNAP can turn the boat into the player.
    bool frontHitsLand = checkAllIslandsCollision(islandManager, fwdPos, boat->radius);
    bool behindHitsLand = checkAllIslandsCollision(islandManager, bwdPos, boat->radius);

    if (upp && frontHitsLand)
        pushedLand = true;
    else if (upp && !frontBlocked) {
        boat->position.x -= sinf(boat->yaw) * boat->speed;
        boat->position.z += cosf(boat->yaw) * boat->speed;
    }

    if (down && behindHitsLand)
        pushedLand = true;
    else if (down && !behindBlocked) {
        boat->position.x += sinf(boat->yaw) * boat->speed;
        boat->position.z -= cosf(boat->yaw) * boat->speed;
    }

    // --- Rotation (always allowed) ---
    if (left)  boat->yaw -= 0.05f;
    if (right) boat->yaw += 0.05f;

    return pushedLand;
}

// ============================================================
// PLAYER_SNAP launch helper
// ============================================================

void boatLaunch(Boat* boat, float x, float z, float yaw, IslandManager* islandManager)
{
    (void)islandManager;

    // Start just beyond the shoreline in the direction the player was
    // travelling. The next frame can then sail normally.
    const float LAUNCH_DISTANCE = 1.10f;

    boat->position.x = x - sinf(yaw) * LAUNCH_DISTANCE * 2;
    boat->position.z = z + cosf(yaw) * LAUNCH_DISTANCE * 2;
    boat->yaw = yaw;
    boat->position.y = 15.0f;
}

// ============================================================
// Rendering
// ============================================================

void drawBoat(float x, float y, float z, float yaw) {
    const float LEN    = 1.5f;
    const float WIDTH  = 0.5f;
    const float HEIGHT = 0.3f;

    float verts[8][3] = {
        { -WIDTH/2, -HEIGHT/2, -LEN/2 }, // 0
        {  WIDTH/2, -HEIGHT/2, -LEN/2 }, // 1
        {  WIDTH/2,  HEIGHT/2, -LEN/2 }, // 2
        { -WIDTH/2,  HEIGHT/2, -LEN/2 }, // 3
        { -WIDTH/2, -HEIGHT/2,  LEN/2 }, // 4
        {  WIDTH/2, -HEIGHT/2,  LEN/2 }, // 5
        {  WIDTH/2,  HEIGHT/2,  LEN/2 }, // 6
        { -WIDTH/2,  HEIGHT/2,  LEN/2 }  // 7
    };

    float cosY = cosf(yaw), sinY = sinf(yaw);
    for (int i = 0; i < 8; i++) {
        float rx = verts[i][0]*cosY - verts[i][2]*sinY;
        float rz = verts[i][0]*sinY + verts[i][2]*cosY;
        verts[i][0] = rx; verts[i][2] = rz;
    }

    #define V(i,R,G,B) GX_Position3f32(x+verts[i][0], y+verts[i][1], z+verts[i][2]); GX_Color3f32(R,G,B)

    GX_Begin(GX_QUADS, GX_VTXFMT0, 24);
    V(0,1,1,1); V(1,1,1,1); V(2,1,1,1); V(3,1,1,1); // front
    V(4,1,1,1); V(5,1,1,1); V(6,1,1,1); V(7,1,1,1); // back
    V(0,1,1,1); V(3,1,1,1); V(7,1,1,1); V(4,1,1,1); // left
    V(1,1,1,1); V(2,1,1,1); V(6,1,1,1); V(5,1,1,1); // right
    V(3,0.5f,0.5f,0.5f); V(2,0.5f,0.5f,0.5f); V(6,0.5f,0.5f,0.5f); V(7,0.5f,0.5f,0.5f); // deck
    V(0,1,1,1); V(1,1,1,1); V(5,1,1,1); V(4,1,1,1); // keel
    GX_End();

    #undef V

    // Small mast
    float mastTop[3] = { 0.0f, HEIGHT/2 + 1.0f, 0.0f };
    float mx = mastTop[0]*cosY - mastTop[2]*sinY;
    float mz = mastTop[0]*sinY + mastTop[2]*cosY;
    GX_Begin(GX_LINES, GX_VTXFMT0, 2);
    GX_Position3f32(x, y + HEIGHT/2, z); GX_Color3f32(0.6f, 0.4f, 0.2f);
    GX_Position3f32(x + mx, y + mastTop[1], z + mz); GX_Color3f32(0.6f, 0.4f, 0.2f);
    GX_End();
}