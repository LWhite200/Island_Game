// player.c
// On-foot player controller: movement, jumping, and gravity.
//
//   • g_cameraYawOffset: the camera's horizontal orbit angle, adjusted by
//     the C-stick and read by camera.c to compute camera position.
//   • Vertical physics uses islandGroundHeight(), which -- because every
//     island is a hemisphere -- gives the EXACT ground height under the
//     player with one formula. No guessing, no push-up loops.

#include <gccore.h>
#include <math.h>
#include "player.h"

// Global C-stick camera-yaw offset (written here, read in camera.c)
float g_cameraYawOffset = 0.0f;

// ============================================================
// Constants
// ============================================================

#define CSTICK_SPEED  0.04f  // (kept for reference; camera.c owns the actual orbit speed)

// ============================================================
// Initialisation
// ============================================================

void initPlayer(Player* player) {
    player->position  = (guVector){ 0.0f, 5.0f, 0.0f };
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

    // ---- Horizontal movement ----
    // Movement direction is in camera-adjusted world space
    float moveYaw = player->yaw + g_cameraYawOffset;
    if (upp) {
        player->position.x -= sinf(moveYaw) * player->speed;
        player->position.z += cosf(moveYaw) * player->speed;
    }
    if (down) {
        player->position.x += sinf(moveYaw) * player->speed;
        player->position.z -= cosf(moveYaw) * player->speed;
    }

    // ---- Vertical physics ----
    player->yVelocity -= player->gravity;
    float nextY = player->position.y + player->yVelocity;

    // Exact height of whatever dome (if any) is beneath the player right now.
    Vec3 feetXZ = { player->position.x, nextY, player->position.z };
    float groundY = islandGroundHeight(islandManager, feetXZ, player->radius);

    if (nextY <= groundY) {
        // Landed -- snap exactly onto the dome surface.
        player->position.y = groundY;
        player->yVelocity  = 0.0f;
    } else {
        player->position.y = nextY;
        // No island underfoot and still sinking past the sea floor -- stop here.
        if (player->position.y <= ISLAND_BASE_Y) {
            player->position.y = ISLAND_BASE_Y;
            player->yVelocity  = 0.0f;
        }
    }

    // Show boarding indicator near sea level
    if (player->position.y <= boatChangeY) {
        Vec3 curPos = { player->position.x, player->position.y, player->position.z };
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
            GX_Color3f32(0.2f*br, 1.0f*br, 0.2f*br);
        }
    }
    GX_End();
}
