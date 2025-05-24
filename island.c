#include <gccore.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include "common.h"

#define NUM_CTRL_POINTS 8  // Control points around island perimeter
#define NUM_SEGMENTS 32    // Number of vertices around circle (should be > NUM_CTRL_POINTS)

static bool isCreated = false;
static float ctrlRadius[NUM_CTRL_POINTS];
static float ctrlHeight[NUM_CTRL_POINTS];

// Helper: linear interpolate between a and b by t (0..1)
float lerp(float a, float b, float t) {
    return a + t * (b - a);
}

// Clamp index for circular control points
int clampCtrlIndex(int i) {
    int n = NUM_CTRL_POINTS;
    while (i < 0) i += n;
    while (i >= n) i -= n;
    return i;
}

// Catmull-Rom spline interpolation between 4 control points for smooth curves
float catmullRom(float p0, float p1, float p2, float p3, float t) {
    float t2 = t * t;
    float t3 = t2 * t;
    return 0.5f * (
        (2 * p1) +
        (-p0 + p2) * t +
        (2 * p0 - 5 * p1 + 4 * p2 - p3) * t2 +
        (-p0 + 3 * p1 - 3 * p2 + p3) * t3
        );
}

// Initialize island shape with random control points
void generateIslandShape(float baseRadius) {
    for (int i = 0; i < NUM_CTRL_POINTS; ++i) {
        // Radius varies ±30%
        ctrlRadius[i] = baseRadius * (1.0f + ((float)rand() / RAND_MAX - 0.5f) * 0.6f);
        // Height offset ±1.0
        ctrlHeight[i] = ((float)rand() / RAND_MAX - 0.5f) * 2.0f;
    }
}

// Get interpolated radius at angle theta using Catmull-Rom spline of control points
float getInterpolatedRadius(float theta) {
    float t = theta / (2.0f * M_PI) * NUM_CTRL_POINTS;
    int i1 = (int)floorf(t);
    float localT = t - i1;

    int i0 = clampCtrlIndex(i1 - 1);
    int i2 = clampCtrlIndex(i1 + 1);
    int i3 = clampCtrlIndex(i1 + 2);
    i1 = clampCtrlIndex(i1);

    return catmullRom(ctrlRadius[i0], ctrlRadius[i1], ctrlRadius[i2], ctrlRadius[i3], localT);
}

// Get interpolated height offset similarly
float getInterpolatedHeight(float theta) {
    float t = theta / (2.0f * M_PI) * NUM_CTRL_POINTS;
    int i1 = (int)floorf(t);
    float localT = t - i1;

    int i0 = clampCtrlIndex(i1 - 1);
    int i2 = clampCtrlIndex(i1 + 1);
    int i3 = clampCtrlIndex(i1 + 2);
    i1 = clampCtrlIndex(i1);

    return catmullRom(ctrlHeight[i0], ctrlHeight[i1], ctrlHeight[i2], ctrlHeight[i3], localT);
}

// Color gradient based on height (sandy low to green high)
void colorForHeight(float height, float baseY, float* r, float* g, float* b) {
    float t = (height - baseY) / 3.0f;
    if (t < 0) t = 0;
    if (t > 1) t = 1;
    *r = 0.8f * (1.0f - t) + 0.2f * t;
    *g = 0.6f * (1.0f - t) + 0.9f * t;
    *b = 0.3f * (1.0f - t) + 0.3f * t;
}

void drawIsland(float x, float y, float z) {
    const float baseRadius = ISLAND_RADIUS * 2.5f; // bigger island

    if (!isCreated) {
        srand(time(NULL)); // seed randomness by current time
        generateIslandShape(baseRadius);
        isCreated = true;
    }

    for (int i = 0; i < NUM_SEGMENTS; ++i) {
        float theta1 = (i * 2 * M_PI) / NUM_SEGMENTS;
        float theta2 = ((i + 1) * 2 * M_PI) / NUM_SEGMENTS;

        for (int j = 0; j < NUM_SEGMENTS / 2; ++j) {
            float phi1 = (j * M_PI) / NUM_SEGMENTS - M_PI / 2;
            float phi2 = ((j + 1) * M_PI) / NUM_SEGMENTS - M_PI / 2;

            float cosPhi1 = cosf(phi1);
            float cosPhi2 = cosf(phi2);

            // Smoothly interpolated radius from control points
            float r11 = getInterpolatedRadius(theta1) * cosPhi1;
            float r12 = getInterpolatedRadius(theta2) * cosPhi1;
            float r22 = getInterpolatedRadius(theta2) * cosPhi2;
            float r21 = getInterpolatedRadius(theta1) * cosPhi2;

            // Height offset from control points + base hill shape
            float hOffset1 = getInterpolatedHeight(theta1);
            float hOffset2 = getInterpolatedHeight(theta2);

            // Base hill shape: higher near center latitude, lower at edges
            float baseHill1 = 2.0f * (1.0f - cosPhi1 * cosPhi1);
            float baseHill2 = 2.0f * (1.0f - cosPhi2 * cosPhi2);

            float y1 = y + baseHill1 + lerp(hOffset1, hOffset2, 0);
            float y2 = y + baseHill1 + lerp(hOffset1, hOffset2, 1);
            float y3 = y + baseHill2 + lerp(hOffset1, hOffset2, 1);
            float y4 = y + baseHill2 + lerp(hOffset1, hOffset2, 0);

            float x1 = x + r11 * cosf(theta1);
            float z1 = z + r11 * sinf(theta1);

            float x2 = x + r12 * cosf(theta2);
            float z2 = z + r12 * sinf(theta2);

            float x3 = x + r22 * cosf(theta2);
            float z3 = z + r22 * sinf(theta2);

            float x4 = x + r21 * cosf(theta1);
            float z4 = z + r21 * sinf(theta1);

            GX_Begin(GX_QUADS, GX_VTXFMT0, 4);

            float r, g, b;

            colorForHeight(y1, y, &r, &g, &b);
            GX_Position3f32(x1, y1, z1);
            GX_Color3f32(r, g, b);

            colorForHeight(y2, y, &r, &g, &b);
            GX_Position3f32(x2, y2, z2);
            GX_Color3f32(r, g, b);

            colorForHeight(y3, y, &r, &g, &b);
            GX_Position3f32(x3, y3, z3);
            GX_Color3f32(r, g, b);

            colorForHeight(y4, y, &r, &g, &b);
            GX_Position3f32(x4, y4, z4);
            GX_Color3f32(r, g, b);

            GX_End();
        }
    }
}
