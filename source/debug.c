//
// debug.c
//
// Minimal Wii GX debug overlay.
//
// No GRRLIB.
// No textures.
// No TTF.
// No font libraries.
//
// The @ symbol is a tiny 5x7 bitmap made from rectangles.
//

#include "debug.h"

#include <gccore.h>
#include <ogc/pad.h>
#include <stdbool.h>


// ============================================================
// Debug menu state
// ============================================================

static bool s_debugVisible = false;


// ============================================================
// Tiny 5x7 bitmap for '@'
//
// Each byte is one row.
// 1 = pixel
// 0 = empty
// ============================================================

static const unsigned char FONT_AT[7] =
{
    0b01110,
    0b10001,
    0b10111,
    0b10101,
    0b10111,
    0b10000,
    0b01111
};


// ============================================================
// Draw a solid 2D rectangle
//
// IMPORTANT:
// Your gx_utils configures GX_VA_CLR0 as GX_RGB8.
//
// Therefore we use GX_Color3u8(), NOT GX_Color4u8().
//
// We also do NOT change the vertex descriptors here.
// They are already configured by init_graphics().
// ============================================================

static void drawRect2D(
    f32 x1,
    f32 y1,
    f32 x2,
    f32 y2,
    u8 r,
    u8 g,
    u8 b
)
{
    GX_Begin(GX_QUADS, GX_VTXFMT0, 4);

        GX_Position3f32(x1, y1, 0.0f);
        GX_Color3u8(r, g, b);

        GX_Position3f32(x2, y1, 0.0f);
        GX_Color3u8(r, g, b);

        GX_Position3f32(x2, y2, 0.0f);
        GX_Color3u8(r, g, b);

        GX_Position3f32(x1, y2, 0.0f);
        GX_Color3u8(r, g, b);

    GX_End();
}


// ============================================================
// Draw the bitmap @
//
// The original glyph is only 5x7 pixels.
// "scale" controls how large each pixel becomes.
// ============================================================

static void drawAt(
    f32 x,
    f32 y,
    f32 scale
)
{
    for (int row = 0; row < 7; row++)
    {
        unsigned char bits = FONT_AT[row];

        for (int col = 0; col < 5; col++)
        {
            if (bits & (1 << (4 - col)))
            {
                f32 px = x + (col * scale);
                f32 py = y + (row * scale);

                drawRect2D(
                    px,
                    py,
                    px + scale,
                    py + scale,
                    255,
                    255,
                    255
                );
            }
        }
    }
}


// ============================================================
// Initialize
// ============================================================

void initDebugMenu(void)
{
    s_debugVisible = false;
}


// ============================================================
// Update
// ============================================================

void updateDebugMenu(void)
{
    if (PAD_ButtonsDown(0) & PAD_BUTTON_Y)
    {
        s_debugVisible = !s_debugVisible;
    }
}


// ============================================================
// Draw
// ============================================================

void drawDebugMenu(void)
{
    if (!s_debugVisible)
        return;


    // --------------------------------------------------------
    // Set up a simple 640x480 orthographic projection.
    //
    // This is only used while drawing the debug overlay.
    // --------------------------------------------------------

    Mtx44 ortho;

    guOrtho(
        ortho,
        0.0f,     // top
        480.0f,   // bottom
        0.0f,     // left
        640.0f,   // right
        0.0f,
        1.0f
    );

    GX_LoadProjectionMtx(
        ortho,
        GX_ORTHOGRAPHIC
    );


    // --------------------------------------------------------
    // Identity matrix.
    //
    // This means our X/Y coordinates above are screen
    // coordinates instead of world coordinates.
    // --------------------------------------------------------

    Mtx identity;

    guMtxIdentity(identity);

    GX_LoadPosMtxImm(
        identity,
        GX_PNMTX0
    );


    // --------------------------------------------------------
    // Turn off depth testing.
    //
    // This makes the debug menu appear on top of the game.
    // --------------------------------------------------------

    GX_SetZMode(
        GX_FALSE,
        GX_ALWAYS,
        GX_FALSE
    );


    // --------------------------------------------------------
    // Black debug window.
    // --------------------------------------------------------

    drawRect2D(
        200.0f,
        100.0f,
        440.0f,
        380.0f,
        0,
        0,
        0
    );


    // --------------------------------------------------------
    // White @ symbol.
    // --------------------------------------------------------

    drawAt(
        300.0f,
        210.0f,
        20.0f
    );


    // --------------------------------------------------------
    // Restore normal depth testing.
    //
    // We don't restore the projection here because this is
    // drawn at the END of the 3D scene, immediately before
    // end_frame(). The next frame's setup_camera() replaces
    // the projection anyway.
    // --------------------------------------------------------

    GX_SetZMode(
        GX_TRUE,
        GX_LEQUAL,
        GX_TRUE
    );
}