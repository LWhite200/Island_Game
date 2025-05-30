#include <gccore.h>
#include <math.h>
#include "boat.h"


// Initialize boat with default values
void initBoat(Boat* boat) {
    boat->position.x = 0.0f;
    boat->position.y = 0.0f;
    boat->position.z = 0.0f;
    boat->yaw = 0.0f;
    boat->speed = 0.2f;
    boat->radius = 1.0f;
}

// Update boat position based on input
void updateBoat(Boat* boat, bool upp, bool down, bool left, bool right, float time, IslandManager* islandManager) {

    // Compute wave height at boat's current position
    float boatHeight = sinf((boat->position.x + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE +
        cosf((boat->position.z + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE;

    // Current position of the boat (for collision and indicator)
    Vec3 curPos = {
        boat->position.x,
        boatHeight,
        boat->position.z
    };

    // Forward and backward position for collision checking
    Vec3 backwardPos = {
        boat->position.x + sinf(boat->yaw) * boat->speed,
        boatHeight,
        boat->position.z - cosf(boat->yaw) * boat->speed
    };

    Vec3 forwardPos = {
        boat->position.x - sinf(boat->yaw) * boat->speed,
        boatHeight,
        boat->position.z + cosf(boat->yaw) * boat->speed
    };

    // Collision checks
    bool frontBlocked = checkAllIslandsCollision(islandManager, forwardPos, boat->radius);
    bool currentlyBlocked = checkAllIslandsCollision(islandManager, curPos, boat->radius);
    bool behindBlocked = checkAllIslandsCollision(islandManager, backwardPos, boat->radius);

    // Show indicator if near island
    if (frontBlocked || currentlyBlocked || behindBlocked) {
        drawIndicator(curPos);
    }

    // Movement
    if (down && !behindBlocked) {
        boat->position.x += sinf(boat->yaw) * boat->speed;
        boat->position.z -= cosf(boat->yaw) * boat->speed;
    }

    if (upp && !frontBlocked) {
        boat->position.x -= sinf(boat->yaw) * boat->speed;
        boat->position.z += cosf(boat->yaw) * boat->speed;
    }

    // Rotation
    if (left) {
        boat->yaw -= 0.05f;
    }

    if (right) {
        boat->yaw += 0.05f;
    }
}

void drawBoat(float x, float y, float z, float yaw) {
    // Original boat drawing code exactly as you wrote it
    float boatLength = 1.5f;
    float boatWidth = 0.5f;
    float boatHeight = 0.3f;
    float vertices[8][3];

    vertices[0][0] = -boatWidth / 2; vertices[0][1] = -boatHeight / 2; vertices[0][2] = -boatLength / 2;
    vertices[1][0] = boatWidth / 2;  vertices[1][1] = -boatHeight / 2; vertices[1][2] = -boatLength / 2;
    vertices[2][0] = boatWidth / 2;  vertices[2][1] = boatHeight / 2;  vertices[2][2] = -boatLength / 2;
    vertices[3][0] = -boatWidth / 2; vertices[3][1] = boatHeight / 2;  vertices[3][2] = -boatLength / 2;
    vertices[4][0] = -boatWidth / 2; vertices[4][1] = -boatHeight / 2; vertices[4][2] = boatLength / 2;
    vertices[5][0] = boatWidth / 2;  vertices[5][1] = -boatHeight / 2; vertices[5][2] = boatLength / 2;
    vertices[6][0] = boatWidth / 2;  vertices[6][1] = boatHeight / 2;  vertices[6][2] = boatLength / 2;
    vertices[7][0] = -boatWidth / 2; vertices[7][1] = boatHeight / 2;  vertices[7][2] = boatLength / 2;

    for (int i = 0; i < 8; i++) {
        float xTemp = vertices[i][0] * cosf(yaw) - vertices[i][2] * sinf(yaw);
        vertices[i][2] = vertices[i][0] * sinf(yaw) + vertices[i][2] * cosf(yaw);
        vertices[i][0] = xTemp;
    }

    // Draw the 6 faces of the rectangular prism (boat) using quads
    GX_Begin(GX_QUADS, GX_VTXFMT0, 24);  // 6 faces * 4 vertices per face = 24 vertices
    // Front face
    GX_Position3f32(x + vertices[0][0], y + vertices[0][1], z + vertices[0][2]);
    GX_Color3f32(1.0f, 1.0f, 1.0f);  // White color
    GX_Position3f32(x + vertices[1][0], y + vertices[1][1], z + vertices[1][2]);
    GX_Color3f32(1.0f, 1.0f, 1.0f);
    GX_Position3f32(x + vertices[2][0], y + vertices[2][1], z + vertices[2][2]);
    GX_Color3f32(1.0f, 1.0f, 1.0f);
    GX_Position3f32(x + vertices[3][0], y + vertices[3][1], z + vertices[3][2]);
    GX_Color3f32(1.0f, 1.0f, 1.0f);
    // Back face
    GX_Position3f32(x + vertices[4][0], y + vertices[4][1], z + vertices[4][2]);
    GX_Color3f32(1.0f, 1.0f, 1.0f);
    GX_Position3f32(x + vertices[5][0], y + vertices[5][1], z + vertices[5][2]);
    GX_Color3f32(1.0f, 1.0f, 1.0f);
    GX_Position3f32(x + vertices[6][0], y + vertices[6][1], z + vertices[6][2]);
    GX_Color3f32(1.0f, 1.0f, 1.0f);
    GX_Position3f32(x + vertices[7][0], y + vertices[7][1], z + vertices[7][2]);
    GX_Color3f32(1.0f, 1.0f, 1.0f);
    // Left face
    GX_Position3f32(x + vertices[0][0], y + vertices[0][1], z + vertices[0][2]);
    GX_Color3f32(1.0f, 1.0f, 1.0f);
    GX_Position3f32(x + vertices[3][0], y + vertices[3][1], z + vertices[3][2]);
    GX_Color3f32(1.0f, 1.0f, 1.0f);
    GX_Position3f32(x + vertices[7][0], y + vertices[7][1], z + vertices[7][2]);
    GX_Color3f32(1.0f, 1.0f, 1.0f);
    GX_Position3f32(x + vertices[4][0], y + vertices[4][1], z + vertices[4][2]);
    GX_Color3f32(1.0f, 1.0f, 1.0f);
    // Right face
    GX_Position3f32(x + vertices[1][0], y + vertices[1][1], z + vertices[1][2]);
    GX_Color3f32(1.0f, 1.0f, 1.0f);
    GX_Position3f32(x + vertices[2][0], y + vertices[2][1], z + vertices[2][2]);
    GX_Color3f32(1.0f, 1.0f, 1.0f);
    GX_Position3f32(x + vertices[6][0], y + vertices[6][1], z + vertices[6][2]);
    GX_Color3f32(1.0f, 1.0f, 1.0f);
    GX_Position3f32(x + vertices[5][0], y + vertices[5][1], z + vertices[5][2]);
    GX_Color3f32(1.0f, 1.0f, 1.0f);
    // Top face
    GX_Position3f32(x + vertices[3][0], y + vertices[3][1], z + vertices[3][2]);
    GX_Color3f32(0.5f, 0.5f, 0.5f);
    GX_Position3f32(x + vertices[2][0], y + vertices[2][1], z + vertices[2][2]);
    GX_Color3f32(0.5f, 0.5f, 0.5f);
    GX_Position3f32(x + vertices[6][0], y + vertices[6][1], z + vertices[6][2]);
    GX_Color3f32(0.5f, 0.5f, 0.5f);
    GX_Position3f32(x + vertices[7][0], y + vertices[7][1], z + vertices[7][2]);
    GX_Color3f32(0.5f, 0.5f, 0.5f);
    // Bottom face
    GX_Position3f32(x + vertices[0][0], y + vertices[0][1], z + vertices[0][2]);
    GX_Color3f32(1.0f, 1.0f, 1.0f);
    GX_Position3f32(x + vertices[1][0], y + vertices[1][1], z + vertices[1][2]);
    GX_Color3f32(1.0f, 1.0f, 1.0f);
    GX_Position3f32(x + vertices[5][0], y + vertices[5][1], z + vertices[5][2]);
    GX_Color3f32(1.0f, 1.0f, 1.0f);
    GX_Position3f32(x + vertices[4][0], y + vertices[4][1], z + vertices[4][2]);
    GX_Color3f32(1.0f, 1.0f, 1.0f);
    GX_End();
}