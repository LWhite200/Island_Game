// water.c
//
// Chunk-based animated ocean for Wii GX.
//
// The playable world is:
//
//     X = -180 to +180
//     Z = -180 to +180
//
// The ocean itself is effectively infinite.
//
// Each water chunk contains:
//
//     40 x 40 CELLS
//
// Each cell is:
//
//     2 x 2 world units
//
// Therefore each complete chunk is:
//
//     80 x 80 world units
//
// IMPORTANT:
// 40 cells require 41 vertices.
//
// The previous version incorrectly used 40 vertices, which created
// 39 cells = 78 world units, while chunks were placed 80 units apart.
// That caused the visible gaps between chunks.
//
// This version uses:
//     40 cells
//     41 vertices
//     80 world-unit chunks
//
// Wave calculations use WORLD coordinates so neighboring chunks
// share the exact same wave pattern and connect seamlessly.
//
// No textures.
// No GRRLIB.
// No font system.
// GX only.
//

#include <gccore.h>
#include <math.h>

#include "common.h"


// ============================================================
// WATER CONFIGURATION
// ============================================================

// Number of actual CELLS in one chunk.
//
// IMPORTANT:
// A 40-cell grid requires 41 vertices.
#define WATER_CHUNK_CELLS   40


// Size of each water cell in world units.
#define WATER_CELL_SIZE     2.5f


// Complete chunk size.
//
// 40 cells × 2 world units = 80 world units.
#define WATER_CHUNK_SIZE    (WATER_CHUNK_CELLS * WATER_CELL_SIZE)


// ============================================================
// CHUNK RENDER DISTANCE
// ============================================================
//
// Radius 1 means:
//
//     [ ][ ][ ]
//     [ ][P][ ]
//     [ ][ ][ ]
//
// So we render 9 chunks total.
//

#define WATER_CHUNK_RADIUS  1


// ============================================================
// WATER WAVE
// ============================================================
//
// Calculate the height of the water at a WORLD coordinate.
//
// Using world coordinates here is critical.
//
// If each chunk used local coordinates, the wave would restart
// at every chunk boundary.
//

static float getWaterHeight(
    float worldX,
    float worldZ,
    float time
)
{
    float y;

    y =
        sinf(
            (worldX + time) * WAVE_FREQUENCY
        ) * WAVE_AMPLITUDE;

    y +=
        cosf(
            (worldZ + time) * WAVE_FREQUENCY
        ) * WAVE_AMPLITUDE;

    return y;
}


// ============================================================
// DRAW ONE WATER CHUNK
// ============================================================
//
// chunkWorldX and chunkWorldZ are the WORLD coordinates of the
// bottom-left corner of the chunk.
//
// A chunk occupies:
//
//     X = chunkWorldX
//         through
//         chunkWorldX + 80
//
//     Z = chunkWorldZ
//         through
//         chunkWorldZ + 80
//
// Because we have 40 cells, we need 41 vertices per side.
//

static void drawWaterChunk(
    float time,
    float chunkWorldX,
    float chunkWorldZ
)
{
    // --------------------------------------------------------
    // Number of vertices per side
    // --------------------------------------------------------
    //
    // 40 cells require 41 vertices.
    //

    const int VERTICES_PER_SIDE =
        WATER_CHUNK_CELLS + 1;


    // Maximum possible wave height range.
    const float MAX_AMP =
        WAVE_AMPLITUDE * 2.0f;


    int i;
    int j;


    // ========================================================
    // Begin chunk
    // ========================================================
    //
    // 40 x 40 cells
    // = 1600 quads
    //
    // 1600 x 4 vertices
    // = 6400 vertices
    //

    GX_Begin(
        GX_QUADS,
        GX_VTXFMT0,
        WATER_CHUNK_CELLS *
        WATER_CHUNK_CELLS *
        4
    );


    // ========================================================
    // Generate all 40 x 40 cells
    // ========================================================

    for (
        i = 0;
        i < WATER_CHUNK_CELLS;
        i++
    )
    {
        for (
            j = 0;
            j < WATER_CHUNK_CELLS;
            j++
        )
        {
            float x0;
            float z0;

            float x1;
            float z1;

            float x2;
            float z2;

            float x3;
            float z3;

            float y0;
            float y1;
            float y2;
            float y3;

            float avg;
            float t;

            float r;
            float g;
            float b;

            float sh;


            // =================================================
            // Calculate world positions
            // =================================================
            //
            // Notice that i and j can go from 0 through 39.
            //
            // Therefore the final vertex reaches:
            //
            //     40 × 2 = 80
            //
            // giving us a complete 80x80 chunk.
            //

            x0 =
                chunkWorldX +
                (float)i * WATER_CELL_SIZE;

            z0 =
                chunkWorldZ +
                (float)j * WATER_CELL_SIZE;


            x1 =
                x0 +
                WATER_CELL_SIZE;

            z1 =
                z0;


            x2 =
                x0 +
                WATER_CELL_SIZE;

            z2 =
                z0 +
                WATER_CELL_SIZE;


            x3 =
                x0;

            z3 =
                z0 +
                WATER_CELL_SIZE;


            // =================================================
            // Calculate wave heights
            // =================================================
            //
            // WORLD coordinates are used for every vertex.
            //

            y0 =
                getWaterHeight(
                    x0,
                    z0,
                    time
                );

            y1 =
                getWaterHeight(
                    x1,
                    z1,
                    time
                );

            y2 =
                getWaterHeight(
                    x2,
                    z2,
                    time
                );

            y3 =
                getWaterHeight(
                    x3,
                    z3,
                    time
                );


            // =================================================
            // Calculate average wave height
            // =================================================

            avg =
                (y0 + y1 + y2 + y3)
                * 0.25f;


            // =================================================
            // Convert height to 0-1 range
            // =================================================

            t =
                (avg + MAX_AMP)
                /
                (2.0f * MAX_AMP);


            if (t < 0.0f)
                t = 0.0f;

            if (t > 1.0f)
                t = 1.0f;


            // =================================================
            // Ocean color
            // =================================================

            r =
                0.02f +
                0.18f * t;

            g =
                0.08f +
                0.20f * t;

            b =
                0.65f +
                0.30f * t;


            // =================================================
            // Slow shimmer
            // =================================================

            sh =
                sinf(time * 0.08f)
                * 0.07f;


            r =
                fminf(
                    1.0f,
                    fmaxf(
                        0.0f,
                        r + sh
                    )
                );


            g =
                fminf(
                    1.0f,
                    fmaxf(
                        0.0f,
                        g + sh * 0.5f
                    )
                );


            b =
                fminf(
                    1.0f,
                    fmaxf(
                        0.0f,
                        b + sh
                    )
                );


            // =================================================
            // Vertex 0
            // =================================================

            GX_Position3f32(
                x0,
                y0,
                z0
            );

            GX_Color3f32(
                r,
                g,
                b
            );


            // =================================================
            // Vertex 1
            // =================================================

            GX_Position3f32(
                x1,
                y1,
                z1
            );

            GX_Color3f32(
                r,
                g,
                b
            );


            // =================================================
            // Vertex 2
            // =================================================

            GX_Position3f32(
                x2,
                y2,
                z2
            );

            GX_Color3f32(
                r,
                g,
                b
            );


            // =================================================
            // Vertex 3
            // =================================================

            GX_Position3f32(
                x3,
                y3,
                z3
            );

            GX_Color3f32(
                r,
                g,
                b
            );
        }
    }


    // ========================================================
    // Finish chunk
    // ========================================================

    GX_End();
}


// ============================================================
// DRAW WATER
// ============================================================
//
// This is the function your existing game should call:
//
//     drawWater(time, playerX, playerZ);
//
// No other code needs to know about the chunk system.
//

void drawWater(
    float time,
    float originX,
    float originZ
)
{
    int playerChunkX;
    int playerChunkZ;

    int offsetX;
    int offsetZ;


    // ========================================================
    // Find the player's current chunk
    // ========================================================
    //
    // Example:
    //
    //     X =  10  -> chunk  0
    //     X =  79  -> chunk  0
    //     X =  80  -> chunk  1
    //
    // Negative coordinates work correctly because floorf()
    // is used.
    //

    playerChunkX =
        (int)floorf(
            originX / WATER_CHUNK_SIZE
        );


    playerChunkZ =
        (int)floorf(
            originZ / WATER_CHUNK_SIZE
        );


    // ========================================================
    // Render surrounding chunks
    // ========================================================
    //
    // Radius = 1:
    //
    //             Z
    //             ↑
    //
    //       ┌─────┬─────┬─────┐
    //       │     │     │     │
    //       │ -1  │  0  │ +1  │
    //       │     │     │     │
    //       ├─────┼─────┼─────┤
    //       │     │     │     │
    //       │ -1  │  P  │ +1  │
    //       │     │     │     │
    //       ├─────┼─────┼─────┤
    //       │     │     │     │
    //       │ -1  │  0  │ +1  │
    //       │     │     │     │
    //       └─────┴─────┴─────┘
    //
    //             X →
    //

    for (
        offsetX = -WATER_CHUNK_RADIUS;
        offsetX <= WATER_CHUNK_RADIUS;
        offsetX++
    )
    {
        for (
            offsetZ = -WATER_CHUNK_RADIUS;
            offsetZ <= WATER_CHUNK_RADIUS;
            offsetZ++
        )
        {
            int chunkX;
            int chunkZ;

            float chunkWorldX;
            float chunkWorldZ;


            // =================================================
            // Determine chunk coordinates
            // =================================================

            chunkX =
                playerChunkX +
                offsetX;

            chunkZ =
                playerChunkZ +
                offsetZ;


            // =================================================
            // Convert chunk coordinates to world coordinates
            // =================================================
            //
            // Because every chunk is exactly 80 world units
            // wide, neighboring chunks now touch perfectly.
            //
            // Example:
            //
            // Chunk 0:
            //     0 -> 80
            //
            // Chunk 1:
            //     80 -> 160
            //
            // No gap.
            //

            chunkWorldX =
                (float)chunkX *
                WATER_CHUNK_SIZE;


            chunkWorldZ =
                (float)chunkZ *
                WATER_CHUNK_SIZE;


            // =================================================
            // Draw chunk
            // =================================================

            drawWaterChunk(
                time,
                chunkWorldX,
                chunkWorldZ
            );
        }
    }
}