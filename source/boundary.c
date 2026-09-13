// boundary.c
// Renders a continuous, visually appealing wooden rail fence boundary around the world.

#include <gccore.h>
#include <math.h>
#include "common.h"
#include "boundary.h"

void drawWorldBoundary(void) {
    float limit = WORLD_RADIUS;
    float minY = -0.5f;   // Slightly below water
    float maxY =  2.2f;   // Top of fence posts
    float railLow = 0.5f; // Middle rail height
    float railHigh = 1.6f;// Top rail height
    float thickness = 0.25f;
    float spacing = 5.0f; // Distance between posts

    // Colors
    float postR = 0.42f, postG = 0.25f, postB = 0.12f; // Darker wood for posts
    float railR = 0.58f, railG = 0.38f, railB = 0.18f; // Lighter wood for rails

    // --- Helper to draw a single vertical post ---
    #define DRAW_POST(px, pz) \
        GX_Begin(GX_QUADS, GX_VTXFMT0, 4); \
        GX_Position3f32((px) - thickness, minY, (pz) - thickness); GX_Color3f32(postR, postG, postB); \
        GX_Position3f32((px) + thickness, minY, (pz) - thickness); GX_Color3f32(postR, postG, postB); \
        GX_Position3f32((px) + thickness, maxY, (pz) - thickness); GX_Color3f32(postR, postG, postB); \
        GX_Position3f32((px) - thickness, maxY, (pz) - thickness); GX_Color3f32(postR, postG, postB); \
        GX_End(); \
        GX_Begin(GX_QUADS, GX_VTXFMT0, 4); \
        GX_Position3f32((px) - thickness, minY, (pz) + thickness); GX_Color3f32(postR, postG, postB); \
        GX_Position3f32((px) + thickness, minY, (pz) + thickness); GX_Color3f32(postR, postG, postB); \
        GX_Position3f32((px) + thickness, maxY, (pz) + thickness); GX_Color3f32(postR, postG, postB); \
        GX_Position3f32((px) - thickness, maxY, (pz) + thickness); GX_Color3f32(postR, postG, postB); \
        GX_End()

    // --- Helper to draw continuous horizontal rails along an X-axis edge (Z is constant) ---
    #define DRAW_RAILS_X(z_val) \
        /* Lower Rail */ \
        GX_Begin(GX_QUADS, GX_VTXFMT0, 4); \
        GX_Position3f32(-limit, railLow - thickness, (z_val) - thickness); GX_Color3f32(railR, railG, railB); \
        GX_Position3f32( limit, railLow - thickness, (z_val) - thickness); GX_Color3f32(railR, railG, railB); \
        GX_Position3f32( limit, railLow + thickness, (z_val) - thickness); GX_Color3f32(railR, railG, railB); \
        GX_Position3f32(-limit, railLow + thickness, (z_val) - thickness); GX_Color3f32(railR, railG, railB); \
        GX_End(); \
        /* Upper Rail */ \
        GX_Begin(GX_QUADS, GX_VTXFMT0, 4); \
        GX_Position3f32(-limit, railHigh - thickness, (z_val) - thickness); GX_Color3f32(railR, railG, railB); \
        GX_Position3f32( limit, railHigh - thickness, (z_val) - thickness); GX_Color3f32(railR, railG, railB); \
        GX_Position3f32( limit, railHigh + thickness, (z_val) - thickness); GX_Color3f32(railR, railG, railB); \
        GX_Position3f32(-limit, railHigh + thickness, (z_val) - thickness); GX_Color3f32(railR, railG, railB); \
        GX_End()

    // --- Helper to draw continuous horizontal rails along a Z-axis edge (X is constant) ---
    #define DRAW_RAILS_Z(x_val) \
        /* Lower Rail */ \
        GX_Begin(GX_QUADS, GX_VTXFMT0, 4); \
        GX_Position3f32((x_val) - thickness, railLow - thickness, -limit); GX_Color3f32(railR, railG, railB); \
        GX_Position3f32((x_val) - thickness, railLow - thickness,  limit); GX_Color3f32(railR, railG, railB); \
        GX_Position3f32((x_val) - thickness, railLow + thickness,  limit); GX_Color3f32(railR, railG, railB); \
        GX_Position3f32((x_val) - thickness, railLow + thickness, -limit); GX_Color3f32(railR, railG, railB); \
        GX_End(); \
        /* Upper Rail */ \
        GX_Begin(GX_QUADS, GX_VTXFMT0, 4); \
        GX_Position3f32((x_val) - thickness, railHigh - thickness, -limit); GX_Color3f32(railR, railG, railB); \
        GX_Position3f32((x_val) - thickness, railHigh - thickness,  limit); GX_Color3f32(railR, railG, railB); \
        GX_Position3f32((x_val) - thickness, railHigh + thickness,  limit); GX_Color3f32(railR, railG, railB); \
        GX_Position3f32((x_val) - thickness, railHigh + thickness, -limit); GX_Color3f32(railR, railG, railB); \
        GX_End()

    // 1. Draw continuous horizontal rails for all 4 sides
    DRAW_RAILS_X(-limit);
    DRAW_RAILS_X( limit);
    DRAW_RAILS_Z(-limit);
    DRAW_RAILS_Z( limit);

    // 2. Draw vertical posts along each edge at regular intervals
    for (float coord = -limit; coord <= limit; coord += spacing) {
        DRAW_POST(-limit, coord); // -X Edge
        DRAW_POST( limit, coord); // +X Edge
        DRAW_POST(coord, -limit); // -Z Edge
        DRAW_POST(coord,  limit); // +Z Edge
    }

    #undef DRAW_POST
    #undef DRAW_RAILS_X
    #undef DRAW_RAILS_Z
}