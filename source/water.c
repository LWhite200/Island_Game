// water.c
//
// Single fixed ocean grid for Wii GX.
//
// The playable world and water boundaries are:
//
//     X = -90 to +90
//     Z = -90 to +90
//
// No chunking, no infinite scrolling — just one stationary water plane.
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

#define WATER_MIN_COORD    -90.0f
#define WATER_MAX_COORD     90.0f
#define WATER_CELL_SIZE       2.5f

#define WATER_GRID_CELLS \
    ((int)((WATER_MAX_COORD - WATER_MIN_COORD) / WATER_CELL_SIZE))


// ============================================================
// WATER WAVE
// ============================================================

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
// DRAW WATER
// ============================================================

void drawWater(
    float time,
    float originX,
    float originZ
)
{
    (void)originX;
    (void)originZ;

    // --------------------------------------------------------
    // Keep time small.
    //
    // The wave repeats every:
    //
    //     2 * PI / WAVE_FREQUENCY
    //
    // This prevents the value passed into sinf/cosf from
    // growing indefinitely.
    // --------------------------------------------------------

    const float wavePeriod =
        (2.0f * (float)M_PI) / WAVE_FREQUENCY;

    time = fmodf(time, wavePeriod);

    if (time < 0.0f)
        time += wavePeriod;


    const float MAX_AMP =
        WAVE_AMPLITUDE * 2.0f;

    int i;
    int j;


    // --------------------------------------------------------
    // Calculate the lighting/shading animation once.
    // --------------------------------------------------------

    float sh =
        sinf(time * 0.08f)
        * 0.07f;


    // --------------------------------------------------------
    // Draw rows
    // --------------------------------------------------------

    for (j = 0; j < WATER_GRID_CELLS; j++)
    {
        float z0 =
            WATER_MIN_COORD +
            (float)j * WATER_CELL_SIZE;

        float z1 = z0;

        float z2 =
            z0 + WATER_CELL_SIZE;

        float z3 = z2;


        GX_Begin(
            GX_QUADS,
            GX_VTXFMT0,
            WATER_GRID_CELLS * 4
        );


        for (i = 0; i < WATER_GRID_CELLS; i++)
        {
            float x0 =
                WATER_MIN_COORD +
                (float)i * WATER_CELL_SIZE;

            float x1 =
                x0 + WATER_CELL_SIZE;

            float x2 = x1;

            float x3 = x0;


            // ------------------------------------------------
            // Wave heights
            // ------------------------------------------------

            float y0 =
                getWaterHeight(
                    x0, z0, time
                );

            float y1 =
                getWaterHeight(
                    x1, z1, time
                );

            float y2 =
                getWaterHeight(
                    x2, z2, time
                );

            float y3 =
                getWaterHeight(
                    x3, z3, time
                );


            // ------------------------------------------------
            // Average height for color
            // ------------------------------------------------

            float avg =
                (y0 + y1 + y2 + y3)
                * 0.25f;


            float t =
                (avg + MAX_AMP)
                /
                (2.0f * MAX_AMP);


            if (t < 0.0f)
                t = 0.0f;

            if (t > 1.0f)
                t = 1.0f;


            // ------------------------------------------------
            // Water color
            // ------------------------------------------------

            float r =
                0.02f +
                0.18f * t;

            float g =
                0.08f +
                0.20f * t;

            float b =
                0.65f +
                0.30f * t;


            r = fminf(
                1.0f,
                fmaxf(0.0f, r + sh)
            );

            g = fminf(
                1.0f,
                fmaxf(0.0f, g + sh * 0.5f)
            );

            b = fminf(
                1.0f,
                fmaxf(0.0f, b + sh)
            );


            // ------------------------------------------------
            // Vertex 0
            // ------------------------------------------------

            GX_Position3f32(
                x0, y0, z0
            );

            GX_Color3f32(
                r, g, b
            );


            // ------------------------------------------------
            // Vertex 1
            // ------------------------------------------------

            GX_Position3f32(
                x1, y1, z1
            );

            GX_Color3f32(
                r, g, b
            );


            // ------------------------------------------------
            // Vertex 2
            // ------------------------------------------------

            GX_Position3f32(
                x2, y2, z2
            );

            GX_Color3f32(
                r, g, b
            );


            // ------------------------------------------------
            // Vertex 3
            // ------------------------------------------------

            GX_Position3f32(
                x3, y3, z3
            );

            GX_Color3f32(
                r, g, b
            );
        }


        GX_End();
    }
}