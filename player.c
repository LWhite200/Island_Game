#include <gccore.h>
#include <math.h>
#include "player.h"


// Initialize player with default values
void initPlayer(Player* player) {
    player->position.x = 0.0f;
    player->position.y = 0.0f;
    player->position.z = 0.0f;
    player->yaw = 0.0f;
    player->speed = 0.2f;
    player->radius = 0.25f;
    player->gravity = 0.09;
    player->upPush = 0.001; // when they are in the ground, push up until slightly not
}

// Update player position based on input (same as boat for now)
void updatePlayer(Player* player, bool upp, bool down, bool left, bool right, float time, IslandManager* islandManager) {

    // Current position of the player (for collision and indicator)
    Vec3 curPos = {
        player->position.x,
        player->position.y,
        player->position.z
    };

    // Forward and backward position for collision checking
    Vec3 backwardPos = {
        player->position.x + sinf(player->yaw) * player->speed,
        player->position.y,
        player->position.z - cosf(player->yaw) * player->speed
    };

    Vec3 forwardPos = {
        player->position.x - sinf(player->yaw) * player->speed,
        player->position.y,
        player->position.z + cosf(player->yaw) * player->speed
    };

    // Collision checks
    bool frontBlocked = checkAllIslandsCollision(islandManager, forwardPos, player->radius);
    bool currentlyBlocked = checkAllIslandsCollision(islandManager, curPos, player->radius);
    bool behindBlocked = checkAllIslandsCollision(islandManager, backwardPos, player->radius);

    // Show indicator if near island
    if (frontBlocked || currentlyBlocked || behindBlocked) {
        drawIndicator(curPos);
    }

    // Draw closest area where player could collide with
    if (frontBlocked && !currentlyBlocked) {
        drawIslandHitArea(islandManager, forwardPos, player->radius);
    }
    else {
        drawIslandHitArea(islandManager, curPos, player->radius);
    }

    // Movement
    if (down) {
        player->position.x += sinf(player->yaw) * player->speed;
        player->position.z -= cosf(player->yaw) * player->speed;
    }

    if (upp) {
        player->position.x -= sinf(player->yaw) * player->speed;
        player->position.z += cosf(player->yaw) * player->speed;
    }

    // Rotation
    if (left) {
        player->yaw -= 0.05f;
    }

    if (right) {
        player->yaw += 0.05f;
    }

    Vec3 belowPos = {
        player->position.x - sinf(player->yaw) * player->speed,
        player->position.y - player->gravity,
        player->position.z + cosf(player->yaw) * player->speed
    };

    bool belowBlocked = checkAllIslandsCollision(islandManager, belowPos, player->radius);

    if (!belowBlocked) {
        player->position.y -= player->gravity;
        if (player->position.y <= -2.0) {
            player->position.y = -2.0;
        }
    }
    else {
        while (checkAllIslandsCollision(islandManager, belowPos, player->radius)) {
            belowPos.y += player->upPush;
        }
        player->position.y = belowPos.y;
    }
}

// Easy-to-modify color (RGB)
static float playerColor[3] = { 0.2f, 1.0f, 0.2f };  // Light green

// Pyramid vertices (4 base corners + 1 top)
static float pyramidVertices[5][3] = {
    { 0.0f,  0.5f,  0.0f},  // Top (apex)
    {-0.3f, -0.3f,  0.3f},  // Front-left
    { 0.3f, -0.3f,  0.3f},  // Front-right
    { 0.3f, -0.3f, -0.3f},  // Back-right
    {-0.3f, -0.3f, -0.3f}   // Back-left
};

// Base face indices (quad split into 2 triangles)
static int baseFaces[2][3] = {
    {1, 2, 3},  // First triangle of base
    {1, 3, 4}   // Second triangle of base
};

// Side face indices (4 triangles)
static int sideFaces[4][3] = {
    {0, 1, 2},  // Front
    {0, 2, 3},  // Right
    {0, 3, 4},  // Back
    {0, 4, 1}   // Left
};

void drawPlayer(float x, float y, float z, float yaw) {
    float rotated[5][3];

    // Apply yaw rotation to each vertex
    for (int i = 0; i < 5; i++) {
        float px = pyramidVertices[i][0];
        float pz = pyramidVertices[i][2];

        rotated[i][0] = px * cosf(yaw) - pz * sinf(yaw);
        rotated[i][1] = pyramidVertices[i][1];
        rotated[i][2] = px * sinf(yaw) + pz * cosf(yaw);
    }

    // Draw base (2 triangles)
    GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 6);
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 3; j++) {
            int vi = baseFaces[i][j];
            GX_Position3f32(x + rotated[vi][0], y + rotated[vi][1], z + rotated[vi][2]);
            GX_Color3f32(0.1f, 0.1f, 0.1f);  // Dark gray base
        }
    }
    GX_End();

    // Draw 4 triangular sides
    GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 12);
    for (int i = 0; i < 4; i++) {
        float brightness = 0.8f - 0.1f * i;  // Vary color slightly
        for (int j = 0; j < 3; j++) {
            int vi = sideFaces[i][j];
            GX_Position3f32(x + rotated[vi][0], y + rotated[vi][1], z + rotated[vi][2]);
            GX_Color3f32(playerColor[0] * brightness,
                playerColor[1] * brightness,
                playerColor[2] * brightness);
        }
    }
    GX_End();
}
