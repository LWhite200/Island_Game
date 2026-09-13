// water.c
// Draws an animated ocean mesh that follows the player/camera so it appears
// infinite. The grid is centred on `originX, originZ` (the player's current
// XZ position) every frame, so the water never runs out no matter how far
// the player explores.
//
// The wave formula uses world-space coordinates (not grid-local ones) so the
// wave pattern is seamless across re-centerings -- no visible pop when the
// grid shifts.

#include <gccore.h>
#include <math.h>
#include "common.h"

// Size of the water grid in world-units per side.
// At WATER_CELL_SIZE=2 and WORLD_RADIUS=80 this covers a 160x160 patch,
// large enough that the horizon clips it before the edge is visible.
#define WATER_CELL_SIZE    2.0f // World units per cell edge

void drawWater(float time, float originX, float originZ) {
    // Snap the grid origin to cell-sized steps so the grid doesn't slide
    // continuously (which would look odd). Snapping to whole cell widths
    // means the grid shifts discretely, which is invisible to the player.
    float snapX = floorf(originX / WATER_CELL_SIZE) * WATER_CELL_SIZE;
    float snapZ = floorf(originZ / WATER_CELL_SIZE) * WATER_CELL_SIZE;

    int   N    = WORLD_RADIUS;
    float half = (N / 2) * WATER_CELL_SIZE;

    // Maximum wave amplitude used for the colour budget
    const float MAX_AMP = WAVE_AMPLITUDE * 2.0f;

    GX_Begin(GX_QUADS, GX_VTXFMT0, (N - 1) * (N - 1) * 4);

    for (int i = 0; i < N - 1; i++) {
        for (int j = 0; j < N - 1; j++) {
            // World-space XZ of the four quad corners
            float x0 = snapX - half + (float)i       * WATER_CELL_SIZE;
            float z0 = snapZ - half + (float)j       * WATER_CELL_SIZE;
            float x1 = x0 + WATER_CELL_SIZE;
            float z1 = z0;
            float x2 = x0 + WATER_CELL_SIZE;
            float z2 = z0 + WATER_CELL_SIZE;
            float x3 = x0;
            float z3 = z0 + WATER_CELL_SIZE;

            // Wave heights computed in world space so they are seamless
            float y0 = sinf((x0 + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE
                     + cosf((z0 + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE;
            float y1 = sinf((x1 + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE
                     + cosf((z1 + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE;
            float y2 = sinf((x2 + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE
                     + cosf((z2 + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE;
            float y3 = sinf((x3 + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE
                     + cosf((z3 + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE;

            // Colour from average height
            float avg = (y0 + y1 + y2 + y3) * 0.25f;
            float t   = (avg + MAX_AMP) / (2.0f * MAX_AMP); // 0=trough, 1=crest
            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;

            float r = 0.02f + 0.18f * t;
            float g = 0.08f + 0.20f * t;
            float b = 0.65f + 0.30f * t;

            // Slow shimmer
            float sh = sinf(time * 0.08f) * 0.07f;
            r = fminf(1.0f, fmaxf(0.0f, r + sh));
            g = fminf(1.0f, fmaxf(0.0f, g + sh * 0.5f));
            b = fminf(1.0f, fmaxf(0.0f, b + sh));

            GX_Position3f32(x0, y0, z0); GX_Color3f32(r, g, b);
            GX_Position3f32(x1, y1, z1); GX_Color3f32(r, g, b);
            GX_Position3f32(x2, y2, z2); GX_Color3f32(r, g, b);
            GX_Position3f32(x3, y3, z3); GX_Color3f32(r, g, b);
        }
    }

    GX_End();
}
