//
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

// How far below the water-transition height (boatChangeY) the player may wade
// before being stopped.  The boat swap charges the whole time they are in the
// water, so a bigger number = a longer walk into the sea before the swap.
#define PLAYER_WADE_DEPTH     0.6f

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
    // The player may wade a little way into the water.  While they are in it
    // and heading deeper (or are stopped at the wading limit) we report the
    // contact, and main.c turns that into the boat after a short delay.
    float currentFeetY = player->position.y - PLAYER_FOOT_OFFSET;
    float wadeLimitY   = boatChangeY - PLAYER_WADE_DEPTH;

    // Are the player's feet in the water right now?
    Vec3 hereRayStart = { player->position.x, currentFeetY + PLAYER_STEP_UP, player->position.z };
    float hereGroundY = islandGroundHeight(islandManager, hereRayStart, player->radius);
    bool  inWater     = (hereGroundY == ISLAND_NO_GROUND || hereGroundY < boatChangeY);

    Vec3 fwdRayStart = { fwdPos.x, currentFeetY + PLAYER_STEP_UP, fwdPos.z };
    float fwdGroundY = islandGroundHeight(islandManager, fwdRayStart, player->radius);
    bool frontLimit = (fwdGroundY == ISLAND_NO_GROUND || fwdGroundY < wadeLimitY);
    if (frontLimit)
        frontBlocked = true;
    // Deeper = the ground ahead is in the water and not higher than here.
    bool frontDeeper = (fwdGroundY != ISLAND_NO_GROUND &&
                        fwdGroundY < boatChangeY && fwdGroundY <= hereGroundY);
    if (upp && (frontLimit || (inWater && frontDeeper)))
        touchedWater = true;

    Vec3 bwdRayStart = { bwdPos.x, currentFeetY + PLAYER_STEP_UP, bwdPos.z };
    float bwdGroundY = islandGroundHeight(islandManager, bwdRayStart, player->radius);
    bool behindLimit = (bwdGroundY == ISLAND_NO_GROUND || bwdGroundY < wadeLimitY);
    if (behindLimit)
        behindBlocked = true;
    bool behindDeeper = (bwdGroundY != ISLAND_NO_GROUND &&
                         bwdGroundY < boatChangeY && bwdGroundY <= hereGroundY);
    if (down && (behindLimit || (inWater && behindDeeper)))
        touchedWater = true;

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
    const float LAND_AHEAD_MIN  = 2.0f;
    const float LAND_AHEAD_MAX  = 12.0f;
    const float LAND_AHEAD_STEP = 0.5f;

    float x = boatX;
    float z = boatZ;
    float groundY = ISLAND_NO_GROUND;

    for (float ahead = LAND_AHEAD_MIN; ahead <= LAND_AHEAD_MAX; ahead += LAND_AHEAD_STEP) {
        x = boatX - sinf(boatYaw) * ahead;
        z = boatZ + cosf(boatYaw) * ahead;

        Vec3 rayStart = {
            x,
            boatChangeY + 25.0f,
            z
        };

        groundY = islandGroundHeight(
            islandManager,
            rayStart,
            player->radius
        );

        if (groundY != ISLAND_NO_GROUND && groundY >= boatChangeY)
            break;
    }

    if (groundY == ISLAND_NO_GROUND || groundY < boatChangeY)
        return false;

    Vec3 landPos = {
        x,
        groundY + PLAYER_FOOT_OFFSET,
        z
    };

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
    {  0.0f,  0.5f,  0.0f }, // 0: Apex (head/top)
    { -0.3f, -0.3f,  0.3f }, // 1: Front-left
    {  0.3f, -0.3f,  0.3f }, // 2: Front-right
    {  0.3f, -0.3f, -0.3f }, // 3: Back-right
    { -0.3f, -0.3f, -0.3f }, // 4: Back-left
};

// Vibrant, multi-colored palette for each vertex
static const float s_vertColors[5][3] = {
    { 1.0f, 0.85f, 0.2f }, // Apex: Golden Yellow
    { 0.1f, 0.90f, 0.8f }, // Vertex 1: Teal / Cyan
    { 0.9f, 0.2f,  0.6f }, // Vertex 2: Magenta / Pink
    { 0.2f, 0.2f,  1.0f }, // Vertex 3: Electric Blue
    { 0.5f, 0.9f,  0.1f }, // Vertex 4: Lime Green
};

static const int s_base[2][3] = { {1,2,3}, {1,3,4} };
static const int s_side[4][3] = { {0,1,2}, {0,2,3}, {0,3,4}, {0,4,1} };

void drawPlayer(float x, float y, float z, float yaw, float darken, float scale) {
    // Colours fade toward 40% brightness as darken goes 0 -> 1.
    const float shade = 1.0f - 0.6f * darken;

    float cosY = cosf(yaw), sinY = sinf(yaw);

    float rv[5][3];
    for (int i = 0; i < 5; i++) {
        rv[i][0] = (s_verts[i][0]*cosY - s_verts[i][2]*sinY) * scale;
        rv[i][1] =  s_verts[i][1] * scale;
        rv[i][2] = (s_verts[i][0]*sinY + s_verts[i][2]*cosY) * scale;
    }

    // Draw base (dark slate/grey blend)
    GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 6);
    for (int f = 0; f < 2; f++)
        for (int v = 0; v < 3; v++) {
            int vi = s_base[f][v];
            GX_Position3f32(x+rv[vi][0], y+rv[vi][1], z+rv[vi][2]);
            GX_Color3f32(0.15f * shade, 0.15f * shade, 0.2f * shade);
        }
    GX_End();

    // Draw multi-colored sides with smooth vertex gradients
    GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 12);
    for (int f = 0; f < 4; f++) {
        for (int v = 0; v < 3; v++) {
            int vi = s_side[f][v];
            GX_Position3f32(x+rv[vi][0], y+rv[vi][1], z+rv[vi][2]);
            GX_Color3f32(
                s_vertColors[vi][0] * shade,
                s_vertColors[vi][1] * shade,
                s_vertColors[vi][2] * shade
            );
        }
    }
    GX_End();
}