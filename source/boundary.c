// boundary.c
//
// Renders a continuous wooden rail fence around the world.
//
// This version is designed for Wii GX:
//   - Uses ONE GX_Begin / GX_End for the entire fence.
//   - Uses real 3D cuboids for posts and rails.
//   - Avoids floating-point accumulation in the post loop.
//   - Keeps vertex counts predictable.
//   - Avoids hundreds of tiny GX draw calls.
//
// The fence consists of:
//   - 4 continuous horizontal rails
//   - 4-sided 3D wooden posts
//   - 2 rails per side
//
// No dynamic memory is used.

#include <gccore.h>
#include <math.h>

#include "common.h"
#include "boundary.h"


// ============================================================
// Fence configuration
// ============================================================

#define FENCE_POST_SPACING 5.0f

// Post dimensions.
// These are half-extents because the cuboid helper uses
// center +/- half-size.
#define POST_HALF_X 0.25f
#define POST_HALF_Y 1.35f
#define POST_HALF_Z 0.25f

// Rail dimensions.
#define RAIL_HALF_THICKNESS 0.20f
#define RAIL_HALF_HEIGHT    0.20f


// ============================================================
// Colors
// ============================================================

// Darker wood for the vertical posts.
#define POST_R 0.42f
#define POST_G 0.25f
#define POST_B 0.12f

// Slightly lighter wood for the horizontal rails.
#define RAIL_R 0.58f
#define RAIL_G 0.38f
#define RAIL_B 0.18f


// ============================================================
// Draw a cuboid aligned to the X/Y/Z axes.
//
// cx, cy, cz = center of the cuboid
// hx, hy, hz = half-size along each axis
//
// This creates all 6 faces.
//
// 24 vertices are emitted:
//   4 vertices x 6 faces
//
// We deliberately use duplicated vertices instead of indexed
// geometry because that is simple and reliable with GX's
// immediate-mode API.
// ============================================================

static void drawCuboid(
    float cx,
    float cy,
    float cz,
    float hx,
    float hy,
    float hz,
    float r,
    float g,
    float b
)
{
    float x0 = cx - hx;
    float x1 = cx + hx;

    float y0 = cy - hy;
    float y1 = cy + hy;

    float z0 = cz - hz;
    float z1 = cz + hz;


    // --------------------------------------------------------
    // Front face
    // --------------------------------------------------------

    GX_Position3f32(x0, y0, z0);
    GX_Color3f32(r, g, b);

    GX_Position3f32(x1, y0, z0);
    GX_Color3f32(r, g, b);

    GX_Position3f32(x1, y1, z0);
    GX_Color3f32(r, g, b);

    GX_Position3f32(x0, y1, z0);
    GX_Color3f32(r, g, b);


    // --------------------------------------------------------
    // Back face
    // --------------------------------------------------------

    GX_Position3f32(x1, y0, z1);
    GX_Color3f32(r, g, b);

    GX_Position3f32(x0, y0, z1);
    GX_Color3f32(r, g, b);

    GX_Position3f32(x0, y1, z1);
    GX_Color3f32(r, g, b);

    GX_Position3f32(x1, y1, z1);
    GX_Color3f32(r, g, b);


    // --------------------------------------------------------
    // Left face
    // --------------------------------------------------------

    GX_Position3f32(x0, y0, z1);
    GX_Color3f32(r, g, b);

    GX_Position3f32(x0, y0, z0);
    GX_Color3f32(r, g, b);

    GX_Position3f32(x0, y1, z0);
    GX_Color3f32(r, g, b);

    GX_Position3f32(x0, y1, z1);
    GX_Color3f32(r, g, b);


    // --------------------------------------------------------
    // Right face
    // --------------------------------------------------------

    GX_Position3f32(x1, y0, z0);
    GX_Color3f32(r, g, b);

    GX_Position3f32(x1, y0, z1);
    GX_Color3f32(r, g, b);

    GX_Position3f32(x1, y1, z1);
    GX_Color3f32(r, g, b);

    GX_Position3f32(x1, y1, z0);
    GX_Color3f32(r, g, b);


    // --------------------------------------------------------
    // Bottom face
    // --------------------------------------------------------

    GX_Position3f32(x0, y0, z1);
    GX_Color3f32(r, g, b);

    GX_Position3f32(x1, y0, z1);
    GX_Color3f32(r, g, b);

    GX_Position3f32(x1, y0, z0);
    GX_Color3f32(r, g, b);

    GX_Position3f32(x0, y0, z0);
    GX_Color3f32(r, g, b);


    // --------------------------------------------------------
    // Top face
    // --------------------------------------------------------

    GX_Position3f32(x0, y1, z0);
    GX_Color3f32(r, g, b);

    GX_Position3f32(x1, y1, z0);
    GX_Color3f32(r, g, b);

    GX_Position3f32(x1, y1, z1);
    GX_Color3f32(r, g, b);

    GX_Position3f32(x0, y1, z1);
    GX_Color3f32(r, g, b);
}


// ============================================================
// Draw the world boundary
// ============================================================

void drawWorldBoundary(void)
{
    const float limit = WORLD_RADIUS;

    // Fence post center.
    //
    // The post extends from:
    //     -0.5
    // to:
    //      2.2
    //
    // Therefore its center is:
    //      (-0.5 + 2.2) / 2 = 0.85
    //
    const float postCenterY = 0.85f;

    // Rails sit at these heights.
    const float railLow  = 0.50f;
    const float railHigh = 1.60f;


    // ========================================================
    // Start ONE large GX batch.
    //
    // This is much better than repeatedly doing:
    //
    //     GX_Begin()
    //     ...
    //     GX_End()
    //
    // for every individual piece of fence.
    //
    // Each cuboid produces:
    //
    //     6 faces * 4 vertices = 24 vertices
    //
    // The number of posts is calculated first so the GX
    // vertex count can be supplied safely.
    // ========================================================

    int postCount;
    int vertexCount;

    postCount = (int)ceilf((limit * 2.0f) / FENCE_POST_SPACING) + 1;

    // Four sides, with one post per coordinate.
    //
    // There are 4 * postCount posts.
    //
    // Each post = 24 vertices.
    //
    // Rails:
    //   4 sides * 2 rails * 24 vertices
    //
    vertexCount =
        (postCount * 4 * 24) +
        (4 * 2 * 24);


    // --------------------------------------------------------
    // Safety clamp.
    //
    // GX_Begin takes a u16 vertex count.
    //
    // In practice WORLD_RADIUS should be nowhere near large
    // enough for this to matter, but this prevents an invalid
    // vertex count from wrapping around if somebody changes
    // WORLD_RADIUS to something enormous later.
    // --------------------------------------------------------

    if (vertexCount > 65535)
        return;


    GX_Begin(GX_QUADS, GX_VTXFMT0, vertexCount);


    // ========================================================
    // RAILS
    // ========================================================

    // --------------------------------------------------------
    // -Z rail
    // --------------------------------------------------------

    drawCuboid(
        0.0f,
        railLow,
        -limit,
        limit,
        RAIL_HALF_HEIGHT,
        RAIL_HALF_THICKNESS,
        RAIL_R,
        RAIL_G,
        RAIL_B
    );

    drawCuboid(
        0.0f,
        railHigh,
        -limit,
        limit,
        RAIL_HALF_HEIGHT,
        RAIL_HALF_THICKNESS,
        RAIL_R,
        RAIL_G,
        RAIL_B
    );


    // --------------------------------------------------------
    // +Z rail
    // --------------------------------------------------------

    drawCuboid(
        0.0f,
        railLow,
        limit,
        limit,
        RAIL_HALF_HEIGHT,
        RAIL_HALF_THICKNESS,
        RAIL_R,
        RAIL_G,
        RAIL_B
    );

    drawCuboid(
        0.0f,
        railHigh,
        limit,
        limit,
        RAIL_HALF_HEIGHT,
        RAIL_HALF_THICKNESS,
        RAIL_R,
        RAIL_G,
        RAIL_B
    );


    // --------------------------------------------------------
    // -X rail
    // --------------------------------------------------------

    drawCuboid(
        -limit,
        railLow,
        0.0f,
        RAIL_HALF_THICKNESS,
        RAIL_HALF_HEIGHT,
        limit,
        RAIL_R,
        RAIL_G,
        RAIL_B
    );

    drawCuboid(
        -limit,
        railHigh,
        0.0f,
        RAIL_HALF_THICKNESS,
        RAIL_HALF_HEIGHT,
        limit,
        RAIL_R,
        RAIL_G,
        RAIL_B
    );


    // --------------------------------------------------------
    // +X rail
    // --------------------------------------------------------

    drawCuboid(
        limit,
        railLow,
        0.0f,
        RAIL_HALF_THICKNESS,
        RAIL_HALF_HEIGHT,
        limit,
        RAIL_R,
        RAIL_G,
        RAIL_B
    );

    drawCuboid(
        limit,
        railHigh,
        0.0f,
        RAIL_HALF_THICKNESS,
        RAIL_HALF_HEIGHT,
        limit,
        RAIL_R,
        RAIL_G,
        RAIL_B
    );


    // ========================================================
    // POSTS
    // ========================================================
    //
    // IMPORTANT:
    //
    // Instead of:
    //
    //     for (float coord = -limit;
    //          coord <= limit;
    //          coord += spacing)
    //
    // we use an INTEGER index.
    //
    // This prevents floating-point accumulation from deciding
    // whether the final post gets rendered.
    // ========================================================

    for (int i = 0; i < postCount; i++)
    {
        float coord;

        coord =
            -limit +
            ((float)i * FENCE_POST_SPACING);


        // ----------------------------------------------------
        // Do not allow the last post to extend beyond the
        // actual world boundary.
        //
        // Instead, clamp it to the edge.
        // ----------------------------------------------------

        if (coord > limit)
            coord = limit;


        // ----------------------------------------------------
        // -X side
        // ----------------------------------------------------

        drawCuboid(
            -limit,
            postCenterY,
            coord,
            POST_HALF_X,
            POST_HALF_Y,
            POST_HALF_Z,
            POST_R,
            POST_G,
            POST_B
        );


        // ----------------------------------------------------
        // +X side
        // ----------------------------------------------------

        drawCuboid(
            limit,
            postCenterY,
            coord,
            POST_HALF_X,
            POST_HALF_Y,
            POST_HALF_Z,
            POST_R,
            POST_G,
            POST_B
        );


        // ----------------------------------------------------
        // -Z side
        // ----------------------------------------------------

        drawCuboid(
            coord,
            postCenterY,
            -limit,
            POST_HALF_X,
            POST_HALF_Y,
            POST_HALF_Z,
            POST_R,
            POST_G,
            POST_B
        );


        // ----------------------------------------------------
        // +Z side
        // ----------------------------------------------------

        drawCuboid(
            coord,
            postCenterY,
            limit,
            POST_HALF_X,
            POST_HALF_Y,
            POST_HALF_Z,
            POST_R,
            POST_G,
            POST_B
        );


        // ----------------------------------------------------
        // Stop once we've reached the opposite boundary.
        //
        // This matters because the final coordinate may have
        // been clamped to 'limit'.
        // ----------------------------------------------------

        if (coord >= limit)
            break;
    }


    // ========================================================
    // Finish the entire fence batch.
    // ========================================================

    GX_End();
}