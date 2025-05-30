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
    player->yVelocity = 0.0f;
    player->radius = 0.3f;
    player->gravity = 0.15;
    player->upPush = 0.075; // when they are in the ground, push up until slightly not
}

// Update player position based on input (same as boat for now)
void updatePlayer(Player* player, bool upp, bool down, bool left, bool right, float time, IslandManager* islandManager) {

    // Current position of the player (for collision and indicator)
    Vec3 curPos = {
        player->position.x,
        player->position.y,
        player->position.z
    };

    Vec3 forwardPos = {
        player->position.x - sinf(player->yaw) * player->speed,
        player->position.y,
        player->position.z + cosf(player->yaw) * player->speed
    };

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

    // -----------------------------------------

    // Apply gravity
    player->yVelocity -= player->gravity;

    // Predict next Y position
    float nextY = player->position.y + player->yVelocity;

    // Check collision at predicted position
    Vec3 bottomPos = {
        player->position.x,
        nextY,
        player->position.z
    };

    bool onGround = checkAllIslandsCollision(islandManager, bottomPos, player->radius);

    if (onGround) {
        // Reset velocity when grounded
        player->yVelocity = 0;

        // Softly push up until no longer colliding (single increment, not while-loop)
        bottomPos.y = player->position.y;
        while (checkAllIslandsCollision(islandManager, bottomPos, player->radius) && bottomPos.y < player->position.y + 0.2f) {
            bottomPos.y += player->upPush;
        }
        player->position.y = bottomPos.y;
    }
    else {
        player->position.y = nextY;

        // Optional clamp for falling limit
        if (player->position.y <= BASE_Y) {
            player->position.y = BASE_Y;
            player->yVelocity = 0;
        }
    }


    // Show indicator if near island
    if (player->position.y <= boatChangeY) {
        drawIndicator(curPos);
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


void drawRadiusSphere(float radius, float cx, float cy, float cz) {
    const int latSteps = 10;
    const int lonSteps = 10;

    float latStep = M_PI / latSteps;
    float lonStep = 2 * M_PI / lonSteps;

    // Horizontal circles (latitude)
    for (int i = 1; i < latSteps; i++) {
        float lat = -M_PI_2 + i * latStep;
        float y = sinf(lat);
        float r = cosf(lat);

        GX_Begin(GX_LINESTRIP, GX_VTXFMT0, lonSteps + 1);
        for (int j = 0; j <= lonSteps; j++) {
            float lon = j * lonStep;
            float x = cosf(lon) * r;
            float z = sinf(lon) * r;

            GX_Position3f32(cx + radius * x, cy + radius * y, cz + radius * z);
            GX_Color3f32(1.0f, 0.0f, 0.0f);  // Red
        }
        GX_End();
    }

    // Vertical circles (longitude)
    for (int j = 0; j < lonSteps; j++) {
        float lon = j * lonStep;

        GX_Begin(GX_LINESTRIP, GX_VTXFMT0, latSteps + 1);
        for (int i = 0; i <= latSteps; i++) {
            float lat = -M_PI_2 + i * latStep;
            float y = sinf(lat);
            float r = cosf(lat);
            float x = cosf(lon) * r;
            float z = sinf(lon) * r;

            GX_Position3f32(cx + radius * x, cy + radius * y, cz + radius * z);
            GX_Color3f32(1.0f, 0.0f, 0.0f);  // Red
        }
        GX_End();
    }
}






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

    drawRadiusSphere(0.3, x, y, z);
}
