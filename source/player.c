// player.c
// On-foot player controller: movement, jumping, gravity, and island collision.
//
//   • g_cameraYawOffset: the camera's horizontal orbit angle, adjusted by
//     the C-stick and read by camera.c to compute camera position.
//   • Island collision uses each island's KD-tree (see kdtree.h) in two ways:
//      1. FLOORS  -- a ray is shot straight down from just above the feet;
//         wherever it hits the mesh is the ground height.
//      2. WALLS   -- a small sphere around the player is pushed out of any
//         steep triangles it overlaps, so you can't walk through cliffs.
//     This works for any island shape, not just perfect domes.

#include <gccore.h>
#include <math.h>
#include "player.h"

// Global C-stick camera-yaw offset (written here, read in camera.c)
float g_cameraYawOffset = 0.0f;

// ============================================================
// Constants
// ============================================================

#define CSTICK_SPEED  0.04f  // (kept for reference; camera.c owns the actual orbit speed)

// How far above the feet the ground ray starts. Bumps up to this height are
// simply stepped onto; anything taller has to be treated as a wall.
#define PLAYER_STEP_UP      0.75f

// A triangle counts as a WALL (blocks walking) when |normal.y| is at or below
// this. 0.7 is roughly a 45 degree slope: steeper than that blocks you,
// gentler than that you can walk up.
#define PLAYER_WALL_MAX_NY  0.7f

// Wall push-out is repeated a few times per frame so that corners (where two
// walls meet) settle properly.
#define PLAYER_PUSH_ITERATIONS 3

// Offset to align player feet (local y = -0.3f) with ground height
#define PLAYER_FOOT_OFFSET  0.3f

// ============================================================
// Initialisation
// ============================================================

void initPlayer(Player* player) {
    player->position  = (guVector){ 0.0f, 25.0f, 0.0f };
    player->yaw       = 0.0f;
    player->speed     = 0.18f;
    player->radius    = 0.3f;
    player->yVelocity = 0.0f;
    player->gravity   = 0.015f;
    g_cameraYawOffset = 0.0f;
}

bool playerIsGrounded(const Player* player) {
    return player->yVelocity == 0.0f;
}

static bool checkWorldBoundary(Vec3 pos, float radius) {
    return (pos.x < -WORLD_RADIUS + radius || pos.x > WORLD_RADIUS - radius ||
            pos.z < -WORLD_RADIUS + radius || pos.z > WORLD_RADIUS - radius);
}

// ============================================================
// Per-frame update
// ============================================================

void updatePlayer(Player* player,
                  bool upp, bool down, bool left, bool right,
                  IslandManager* islandManager)
{
    // ---- Rotation ----
    // Player yaw is always relative to camera yaw so controls feel intuitive
    if (left)  player->yaw -= 0.05f;
    if (right) player->yaw += 0.05f;

    // ---- Horizontal movement setup ----
    float moveYaw = player->yaw + g_cameraYawOffset;
    float moveX = sinf(moveYaw) * player->speed;
    float moveZ = cosf(moveYaw) * player->speed;

    // --- Build candidate positions for collision ---
    Vec3 fwdPos = { player->position.x - moveX, player->position.y, player->position.z + moveZ };
    Vec3 bwdPos = { player->position.x + moveX, player->position.y, player->position.z - moveZ };

    // Check world boundaries using the candidate positions
    bool frontBlocked  = checkWorldBoundary(fwdPos, player->radius);
    bool behindBlocked = checkWorldBoundary(bwdPos, player->radius);

    // ---- Water / Edge Protection ----
    // Prevent walking off the island into the water (when ground drops below indicator threshold)
    float currentFeetY = player->position.y - PLAYER_FOOT_OFFSET;

    Vec3 fwdRayStart = { fwdPos.x, currentFeetY + PLAYER_STEP_UP, fwdPos.z };
    float fwdGroundY = islandGroundHeight(islandManager, fwdRayStart, player->radius);
    if (fwdGroundY == ISLAND_NO_GROUND || fwdGroundY < boatChangeY) {
        frontBlocked = true;
    }

    Vec3 bwdRayStart = { bwdPos.x, currentFeetY + PLAYER_STEP_UP, bwdPos.z };
    float bwdGroundY = islandGroundHeight(islandManager, bwdRayStart, player->radius);
    if (bwdGroundY == ISLAND_NO_GROUND || bwdGroundY < boatChangeY) {
        behindBlocked = true;
    }

    // ---- Horizontal movement execution ----
    if (upp && !frontBlocked) {
        player->position.x -= moveX;
        player->position.z += moveZ;
    }
    if (down && !behindBlocked) {
        player->position.x += moveX;
        player->position.z -= moveZ;
    }

    // ---- Wall collision (KD-tree sphere push-out) ----
    for (int iter = 0; iter < PLAYER_PUSH_ITERATIONS; iter++) {
        Vec3 sphereCentre = { player->position.x,
                              player->position.y,
                              player->position.z };
        Vec3 push;
        if (!islandWallPush(islandManager, sphereCentre, player->radius,
                            PLAYER_WALL_MAX_NY, &push))
            break;                        // not touching any wall: done

        player->position.x += push.x;
        player->position.z += push.z;
    }

    // ---- Vertical physics ----
    player->yVelocity -= player->gravity;
    float nextY = player->position.y + player->yVelocity;

    // Ground height under the player's feet
    currentFeetY = player->position.y - PLAYER_FOOT_OFFSET;
    Vec3 rayStart = { player->position.x,
                      currentFeetY + PLAYER_STEP_UP,
                      player->position.z };
    float groundY = islandGroundHeight(islandManager, rayStart, player->radius);

    if (nextY <= groundY + PLAYER_FOOT_OFFSET) {
        // Landed -- snap onto the surface with foot offset corrected.
        player->position.y = groundY + PLAYER_FOOT_OFFSET;
        player->yVelocity  = 0.0f;
    } else {
        player->position.y = nextY;
        // No island underfoot and still sinking past the sea floor -- stop here.
        if (player->position.y <= ISLAND_BASE_Y + PLAYER_FOOT_OFFSET) {
            player->position.y = ISLAND_BASE_Y + PLAYER_FOOT_OFFSET;
            player->yVelocity  = 0.0f;
        }
    }

    // Show boarding indicator near sea level (check actual feet height)
    if ((player->position.y - PLAYER_FOOT_OFFSET) <= boatChangeY) {
        Vec3 curPos = { player->position.x, player->position.y - PLAYER_FOOT_OFFSET, player->position.z };
        drawIndicator(curPos);
    }
}

// ============================================================
// Rendering
// ============================================================

static const float s_verts[5][3] = {
    {  0.0f,  0.5f,  0.0f },
    { -0.3f, -0.3f,  0.3f },
    {  0.3f, -0.3f,  0.3f },
    {  0.3f, -0.3f, -0.3f },
    { -0.3f, -0.3f, -0.3f },
};
static const int s_base[2][3] = { {1,2,3}, {1,3,4} };
static const int s_side[4][3] = { {0,1,2}, {0,2,3}, {0,3,4}, {0,4,1} };

void drawPlayer(float x, float y, float z, float yaw) {
    float cosY = cosf(yaw), sinY = sinf(yaw);

    float rv[5][3];
    for (int i = 0; i < 5; i++) {
        rv[i][0] = s_verts[i][0]*cosY - s_verts[i][2]*sinY;
        rv[i][1] = s_verts[i][1];
        rv[i][2] = s_verts[i][0]*sinY + s_verts[i][2]*cosY;
    }

    // Base
    GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 6);
    for (int f = 0; f < 2; f++)
        for (int v = 0; v < 3; v++) {
            int vi = s_base[f][v];
            GX_Position3f32(x+rv[vi][0], y+rv[vi][1], z+rv[vi][2]);
            GX_Color3f32(0.08f, 0.1f, 0.08f);
        }
    GX_End();

    // Sides -- plain green player
    GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 12);
    for (int f = 0; f < 4; f++) {
        float br = 0.85f - 0.1f * f;
        for (int v = 0; v < 3; v++) {
            int vi = s_side[f][v];
            GX_Position3f32(x+rv[vi][0], y+rv[vi][1], z+rv[vi][2]);
            GX_Color3f32(0.2f*br, 1.0f*br, 0.4f*br);
        }
    }
    GX_End();
}