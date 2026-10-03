// player.c
// On-foot player controller: movement, jumping, gravity, and island collision.
//
// PLAYER_SNAP support:
//   - returns true when the player's requested movement reaches water
//   - provides playerLandAhead() for the boat -> player transition

#include <gccore.h>
#include <math.h>
#include "player.h"

// Global C-stick camera-yaw offset (written here, read in camera.c)
float g_cameraYawOffset = 0.0f;

// ============================================================
// Constants
// ============================================================

#define CSTICK_SPEED          0.04f
#define PLAYER_STEP_UP        0.75f
#define PLAYER_WALL_MAX_NY    0.7f
#define PLAYER_PUSH_ITERATIONS 3
#define PLAYER_FOOT_OFFSET    0.3f

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

bool updatePlayer(Player* player,
                  bool upp, bool down, bool left, bool right,
                  IslandManager* islandManager)
{
    bool touchedWater = false;

    // ---- Rotation ----
    if (left)  player->yaw -= 0.05f;
    if (right) player->yaw += 0.05f;

    // ---- Horizontal movement setup ----
    float moveYaw = player->yaw + g_cameraYawOffset;
    float moveX = sinf(moveYaw) * player->speed;
    float moveZ = cosf(moveYaw) * player->speed;

    Vec3 fwdPos = {
        player->position.x - moveX,
        player->position.y,
        player->position.z + moveZ
    };
    Vec3 bwdPos = {
        player->position.x + moveX,
        player->position.y,
        player->position.z - moveZ
    };

    // ---- World boundaries ----
    bool frontBlocked  = checkWorldBoundary(fwdPos, player->radius);
    bool behindBlocked = checkWorldBoundary(bwdPos, player->radius);

    // ---- Water / Edge Detection ----
    // Do not actually walk into the water. Instead report the contact so
    // main.c can immediately replace the player with the boat.
    float currentFeetY = player->position.y - PLAYER_FOOT_OFFSET;

    Vec3 fwdRayStart = { fwdPos.x, currentFeetY + PLAYER_STEP_UP, fwdPos.z };
    float fwdGroundY = islandGroundHeight(islandManager, fwdRayStart, player->radius);
    bool frontWater = (fwdGroundY == ISLAND_NO_GROUND || fwdGroundY < boatChangeY);
    if (frontWater) {
        frontBlocked = true;
        if (upp) touchedWater = true;
    }

    Vec3 bwdRayStart = { bwdPos.x, currentFeetY + PLAYER_STEP_UP, bwdPos.z };
    float bwdGroundY = islandGroundHeight(islandManager, bwdRayStart, player->radius);
    bool behindWater = (bwdGroundY == ISLAND_NO_GROUND || bwdGroundY < boatChangeY);
    if (behindWater) {
        behindBlocked = true;
        if (down) touchedWater = true;
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

    // ---- Wall collision ----
    for (int iter = 0; iter < PLAYER_PUSH_ITERATIONS; iter++) {
        Vec3 sphereCentre = {
            player->position.x,
            player->position.y,
            player->position.z
        };
        Vec3 push;

        if (!islandWallPush(islandManager, sphereCentre, player->radius,
                            PLAYER_WALL_MAX_NY, &push))
            break;

        player->position.x += push.x;
        player->position.z += push.z;
    }

    // ---- Vertical physics ----
    player->yVelocity -= player->gravity;
    float nextY = player->position.y + player->yVelocity;

    currentFeetY = player->position.y - PLAYER_FOOT_OFFSET;
    Vec3 rayStart = {
        player->position.x,
        currentFeetY + PLAYER_STEP_UP,
        player->position.z
    };
    float groundY = islandGroundHeight(islandManager, rayStart, player->radius);

    if (groundY != ISLAND_NO_GROUND && nextY <= groundY + PLAYER_FOOT_OFFSET) {
        player->position.y = groundY + PLAYER_FOOT_OFFSET;
        player->yVelocity  = 0.0f;
    } else {
        player->position.y = nextY;

        // No island underfoot: stop at the sea floor rather than falling forever.
        if (player->position.y <= ISLAND_BASE_Y + PLAYER_FOOT_OFFSET) {
            player->position.y = ISLAND_BASE_Y + PLAYER_FOOT_OFFSET;
            player->yVelocity  = 0.0f;
        }
    }

    // Show boarding indicator near sea level.
    if ((player->position.y - PLAYER_FOOT_OFFSET) <= boatChangeY) {
        Vec3 curPos = {
            player->position.x,
            player->position.y - PLAYER_FOOT_OFFSET,
            player->position.z
        };
        drawIndicator(curPos);
    }

    return touchedWater;
}

// ============================================================
// PLAYER_SNAP landing helper
// ============================================================

bool playerLandAhead(Player* player,
                     float boatX, float boatZ, float boatYaw,
                     IslandManager* islandManager)
{
    // The boat has reached land. Place the player slightly forward
    // onto the shoreline so the transition feels seamless.
    const float LAND_AHEAD = 2.0f;

    float x = boatX - sinf(boatYaw) * LAND_AHEAD;
    float z = boatZ + cosf(boatYaw) * LAND_AHEAD;

    // Start a ray well above the shoreline and look for actual island ground.
    Vec3 rayStart = {
        x,
        boatChangeY + 25.0f,
        z
    };

    float groundY = islandGroundHeight(
        islandManager,
        rayStart,
        player->radius
    );

    // No land here, or it is still below the water transition height.
    if (groundY == ISLAND_NO_GROUND || groundY < boatChangeY)
        return false;

    // IMPORTANT:
    // Collision functions use Vec3, while Player.position uses guVector.
    Vec3 landPos = {
        x,
        groundY + PLAYER_FOOT_OFFSET,
        z
    };

    // Push the temporary landing position away from steep shoreline walls.
    Vec3 push;

    if (islandWallPush(
            islandManager,
            landPos,
            player->radius,
            PLAYER_WALL_MAX_NY,
            &push))
    {
        landPos.x += push.x;
        landPos.z += push.z;

        // Verify that the pushed position is still valid land.
        Vec3 verifyRay = {
            landPos.x,
            landPos.y + 2.0f,
            landPos.z
        };

        float verifyGround = islandGroundHeight(
            islandManager,
            verifyRay,
            player->radius
        );

        if (verifyGround == ISLAND_NO_GROUND ||
            verifyGround < boatChangeY)
        {
            return false;
        }

        landPos.y = verifyGround + PLAYER_FOOT_OFFSET;
    }

    // Convert Vec3 -> guVector explicitly.
    player->position.x = landPos.x;
    player->position.y = landPos.y;
    player->position.z = landPos.z;

    player->yaw = boatYaw;
    player->yVelocity = 0.0f;

    return true;
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

    GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 6);
    for (int f = 0; f < 2; f++)
        for (int v = 0; v < 3; v++) {
            int vi = s_base[f][v];
            GX_Position3f32(x+rv[vi][0], y+rv[vi][1], z+rv[vi][2]);
            GX_Color3f32(0.08f, 0.1f, 0.08f);
        }
    GX_End();

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
