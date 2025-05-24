#include <gccore.h>
#include <math.h>

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