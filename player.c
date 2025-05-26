#include <gccore.h>
#include <math.h>

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
