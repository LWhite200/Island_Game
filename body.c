#include <gccore.h>
#include <math.h>
#include "Body.h"


// Initialize Body with default values
void initBody(Body* body, float x, float y, float z) {
    body->position.x = x;
    body->position.y = y;
    body->position.z = z;
    body->yaw = 0.0f;
    body->speed = 0.2f;
    body->radius = 0.3f;
    body->rotationSpeed = 0.05f;
    body->hp = 100;
    body->yVelocity = 0.0f;
    body->gravity = 0.01;
    body->jumpForce = 0.18;
}

// Update Body position based on input
// Update Body position based on input and player tracking
void updateBody(Body* body, IslandManager* islandManager, Vec3 playerPos) {
    // Copy position to a local Vec3
    Vec3 pos = {
        .x = body->position.x,
        .y = body->position.y,
        .z = body->position.z
    };

    // --- Rotation towards player ---
    float dx = playerPos.x - body->position.x;
    float dz = playerPos.z - body->position.z;
    float targetYaw = atan2f(dx, dz);  // Note: Swapped dx and dz to match forward direction

    // Smooth yaw rotation
    float deltaYaw = targetYaw - body->yaw;

    // Normalize deltaYaw to [-π, π]
    while (deltaYaw > M_PI) deltaYaw -= 2 * M_PI;
    while (deltaYaw < -M_PI) deltaYaw += 2 * M_PI;

    if (fabs(deltaYaw) < body->rotationSpeed) {
        body->yaw = targetYaw;
    }
    else {
        body->yaw += (deltaYaw > 0 ? 1 : -1) * body->rotationSpeed;
    }

    // --- Movement toward player if not too close ---
    float distance = sqrtf(dx * dx + dz * dz);
    float minDistance = 2.5f;  // stop when within this distance
    float maxDistance = 15.0f;

    if (maxDistance > distance && distance > minDistance) {
        float timeOffset = (float)(body->position.x + body->position.z);  // use position for wave offset
        float wave = sinf(timeOffset + body->yaw * 4.0f) * 0.1f;

        float moveX = sinf(body->yaw) * (body->speed + wave);
        float moveZ = cosf(body->yaw) * (body->speed + wave);

        body->position.x += moveX;
        body->position.z += moveZ;
    }

    // --- Jumping / bouncing ---
    // Check collision at predicted position
    bool onGround = checkAllIslandsCollision(islandManager, pos, body->radius);

    if (onGround) {
        // Reset velocity when grounded
        body->yVelocity = 0;
        body->position.y = islandGroundHeight(islandManager, pos, body->radius);
    }
    else {
        body->yVelocity -= body->gravity;

        // Optional clamp for falling limit
        body->position.y += body->yVelocity;
        if (body->position.y <= BASE_Y) {
            body->position.y = BASE_Y;
        }
    }
}


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

void drawBody(Body* body) {
    float rotated[5][3];
    static float bodyColor[3] = { 1.0f, 2.0f, 0.2f }; 

    float x = body->position.x;
    float y = body->position.y;
    float z = body->position.z;

    // Apply yaw rotation to each vertex
    for (int i = 0; i < 5; i++) {
        float px = pyramidVertices[i][0];
        float pz = pyramidVertices[i][2];

        rotated[i][0] = px * cosf(body->yaw) - pz * sinf(body->yaw);
        rotated[i][1] = pyramidVertices[i][1];
        rotated[i][2] = px * sinf(body->yaw) + pz * cosf(body->yaw);
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
            GX_Color3f32(bodyColor[0] * brightness, bodyColor[1] * brightness, bodyColor[2] * brightness);
        }
    }
    GX_End();
}