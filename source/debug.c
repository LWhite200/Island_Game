//
// debug.c
//
// In-game debug menu for tuning every value in common.h.
//
// Minimal Wii GX overlay:
//
//     No GRRLIB.
//     No textures.
//     No TTF.
//     No font libraries.
//
// Text is drawn with the tiny 5x7 bitmap font in font.c, using
// small solid GX rectangles.
//
// Controls
// ------------------------------------------------------------
//
//     D-pad UP (menu closed)    open the menu
//
//     D-pad UP / DOWN           pick which value to edit
//     D-pad LEFT / RIGHT        decrease / increase it by 0.01
//                               (hold to repeat)
//                               hold R = x10, hold L = x100
//
//     X                         close, throw the edits away
//     Y                         close, apply the edits and
//                               generate new islands
//
// How it works
// ------------------------------------------------------------
//
// The menu never edits the real g_* variables while it is open.
// It edits a private working copy (s_work). Only Y copies the
// working copy into the real variables. That is what makes X
// "close without changes" -- there is nothing to undo.
//
// To add a new tunable value later: declare it in common.h,
// define it in common.c, then add ONE line to s_items below.
//


#include "debug.h"

#include <gccore.h>
#include <ogc/pad.h>
#include <stdbool.h>
#include <stdio.h>

#include "common.h"
#include "font.h"


// ============================================================
// Menu items
//
// Every row in the menu is one entry in this table.
//
//     label   text shown in the menu
//     type    how the value is edited and displayed
//     target  pointer to the REAL variable in common.c
//     min/max the value is clamped to this range
//
// MAX_ISLANDS and NUM_ISLAND_STYLES are compile-time constants
// in common.h (they size arrays / match the enum), so they are
// used here as limits instead of being editable rows.
// ============================================================

typedef enum
{
    DEBUG_FLOAT,   // steps of 0.01
    DEBUG_INT,     // steps of 1
    DEBUG_BOOL,    // left/right toggles ON / OFF
    DEBUG_STYLE    // an int, shown together with its style name
} DebugItemType;


typedef struct
{
    const char    *label;
    DebugItemType  type;
    void          *target;
    float          min;
    float          max;
} DebugItem;


static const DebugItem s_items[] =
{
    // ---- World / wave ----
    { "WORLD RADIUS",          DEBUG_FLOAT, &g_worldRadius,               10.0f,  500.0f },
    { "WAVE FREQUENCY",        DEBUG_FLOAT, &g_waveFrequency,              0.0f,    5.0f },
    { "WAVE AMPLITUDE",        DEBUG_FLOAT, &g_waveAmplitude,              0.0f,    2.0f },
    { "WAVE SPEED",            DEBUG_FLOAT, &g_waveSpeed,                  0.0f,    0.5f },
    { "BOAT CHANGE Y",         DEBUG_FLOAT, &g_boatChangeY,               -5.0f,    5.0f },
    { "BASE Y",                DEBUG_FLOAT, &g_baseY,                    -20.0f,    5.0f },
    { "ISLAND BASE Y",         DEBUG_FLOAT, &g_islandBaseY,              -20.0f,    5.0f },

    // ---- Island generation ----
    { "NUM ISLANDS",           DEBUG_INT,   &g_numIslands,                 1.0f,  (float)MAX_ISLANDS },
    { "ISLAND RADIUS",         DEBUG_FLOAT, &g_islandDefaultRadius,        1.0f,   60.0f },
    { "ISLAND HEIGHT SCALE",   DEBUG_FLOAT, &g_islandDefaultHeightScale,   0.05f,   5.0f },
    { "ISLAND STYLE",          DEBUG_STYLE, &g_islandDefaultStyle,         0.0f,  (float)(NUM_ISLAND_STYLES - 1) },
    { "ISLAND LON SEGMENTS",   DEBUG_INT,   &g_islandLonSegments,          3.0f,   64.0f },
    { "ISLAND LAT SEGMENTS",   DEBUG_INT,   &g_islandLatSegments,          2.0f,   32.0f },
    { "ISLAND RANDOMNESS",     DEBUG_FLOAT, &g_islandRandomness,           0.0f,    1.0f },

    // ---- Streaming world ----
    { "STREAM DISTANCE",       DEBUG_FLOAT, &g_worldStreamDistance,        1.0f,  500.0f },
    { "CULL DISTANCE",         DEBUG_FLOAT, &g_worldCullDistance,          1.0f, 1000.0f },
    { "ISLAND MIN SEPARATION", DEBUG_FLOAT, &g_islandMinSeparation,        1.0f,  200.0f },

    // ---- Player ----
    { "JUMP FORCE",            DEBUG_FLOAT, &g_jumpForce,                  0.0f,    3.0f },
    { "PLAYER SNAP",           DEBUG_BOOL,  &g_playerSnap,                 0.0f,    1.0f },
};

#define ITEM_COUNT ((int)(sizeof(s_items) / sizeof(s_items[0])))


// Names for the ISLAND STYLE row (same order as IslandColorStyle).
static const char *const s_styleNames[NUM_ISLAND_STYLES] =
{
    "TROPICAL",
    "VOLCANO",
    "ARCTIC"
};


// ============================================================
// Menu state
// ============================================================

static bool  s_open           = false;   // menu on screen?
static bool  s_applyRequested = false;   // Y was pressed, main.c should regenerate
static int   s_selected       = 0;       // highlighted row
static float s_work[ITEM_COUNT];         // values being edited (NOT the real ones)

// Frames each D-pad direction has been held (for auto-repeat).
static int s_holdUp    = 0;
static int s_holdDown  = 0;
static int s_holdLeft  = 0;
static int s_holdRight = 0;

#define REPEAT_DELAY   20   // frames held before repeating starts (~1/3 s)
#define REPEAT_RATE     3   // then fire every 3 frames (~20 per second)


// ============================================================
// Read / write the REAL variables as floats
//
// Every value type is stored as a float inside the working copy
// so the editing code only has to deal with one type.
// (Floats hold small ints and 0/1 exactly.)
// ============================================================

static float readTarget(const DebugItem *item)
{
    switch (item->type)
    {
        case DEBUG_FLOAT:
            return *(const float *)item->target;

        case DEBUG_BOOL:
            return (*(const bool *)item->target) ? 1.0f : 0.0f;

        default:    // DEBUG_INT, DEBUG_STYLE
            return (float)(*(const int *)item->target);
    }
}


static void writeTarget(const DebugItem *item, float value)
{
    switch (item->type)
    {
        case DEBUG_FLOAT:
            *(float *)item->target = value;
            break;

        case DEBUG_BOOL:
            *(bool *)item->target = (value > 0.5f);
            break;

        default:    // DEBUG_INT, DEBUG_STYLE (never negative here)
            *(int *)item->target = (int)(value + 0.5f);
            break;
    }
}


// Real variables -> working copy.  (Done every time the menu opens.)
static void loadWorkingCopy(void)
{
    int i;

    for (i = 0; i < ITEM_COUNT; i++)
        s_work[i] = readTarget(&s_items[i]);
}


// Working copy -> real variables.  (Done when Y is pressed.)
static void applyWorkingCopy(void)
{
    int i;

    for (i = 0; i < ITEM_COUNT; i++)
        writeTarget(&s_items[i], s_work[i]);
}


// ============================================================
// Editing
// ============================================================

// Round to the nearest 0.01 so repeated +/-0.01 steps never
// build up float error (0.15 + 0.01 stays 0.16, not 0.16000001).
static float snapToHundredth(float v)
{
    float scaled = v * 100.0f;
    int   whole  = (int)(scaled + (scaled < 0.0f ? -0.5f : 0.5f));

    return (float)whole / 100.0f;
}


// Change one row of the working copy.
//
//     dir   = -1 (left) or +1 (right)
//     scale = 1, 10 or 100 (R / L held). Only floats use it;
//             ints always move by 1.
static void adjustItem(int index, int dir, int scale)
{
    const DebugItem *item  = &s_items[index];
    float            value = s_work[index];

    switch (item->type)
    {
        case DEBUG_FLOAT:

            // --- HERE ---
            // Make smaller because multiplier
            // 0.01, 0.05, 0.1
            value += (float)dir * 0.01f * (float)scale;
            value  = snapToHundredth(value);
            break;

        case DEBUG_BOOL:
            value = (value > 0.5f) ? 0.0f : 1.0f;   // either direction toggles
            break;

        default:    // DEBUG_INT, DEBUG_STYLE
            value += (float)dir;
            break;
    }

    if (value < item->min) value = item->min;
    if (value > item->max) value = item->max;

    s_work[index] = value;
}


// ============================================================
// D-pad auto-repeat
//
// Returns true on the frame the button was pressed, and then
// again and again while it stays held (after a short delay).
// Without this, going from 80.00 to 100.00 would take 2000
// separate presses.
// ============================================================

static bool buttonRepeat(u32 mask, int *heldFrames)
{
    u32 down = PAD_ButtonsDown(0);
    u32 held = PAD_ButtonsHeld(0);

    if (down & mask)
    {
        *heldFrames = 0;
        return true;
    }

    if (held & mask)
    {
        (*heldFrames)++;

        if (*heldFrames >= REPEAT_DELAY &&
            ((*heldFrames - REPEAT_DELAY) % REPEAT_RATE) == 0)
        {
            return true;
        }

        return false;
    }

    *heldFrames = 0;
    return false;
}


// ============================================================
// Draw a quad's four vertices
//
// IMPORTANT:
// gx_utils configures GX_VA_CLR0 as GX_RGB8.
//
// Therefore we use GX_Color3u8(), NOT GX_Color4u8().
//
// We also do NOT change the vertex descriptors here.
// They are already configured by init_graphics().
//
// This does NOT call GX_Begin / GX_End, so several quads can
// share one GX_Begin (see drawGlyph).
// ============================================================

static void emitQuad(
    f32 x1,
    f32 y1,
    f32 x2,
    f32 y2,
    u8 r,
    u8 g,
    u8 b
)
{
    GX_Position3f32(x1, y1, 0.0f);
    GX_Color3u8(r, g, b);

    GX_Position3f32(x2, y1, 0.0f);
    GX_Color3u8(r, g, b);

    GX_Position3f32(x2, y2, 0.0f);
    GX_Color3u8(r, g, b);

    GX_Position3f32(x1, y2, 0.0f);
    GX_Color3u8(r, g, b);
}


// ============================================================
// Draw a solid 2D rectangle
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

        emitQuad(x1, y1, x2, y2, r, g, b);

    GX_End();
}


// ============================================================
// Draw one 5x7 bitmap glyph
//
// The whole menu is about 500 characters, so this is kept cheap:
//
//   * Lit pixels that touch on the same row are merged into one
//     wide rectangle ("run"). Row 11011 is 2 quads, not 4.
//   * The whole glyph goes out in ONE GX_Begin / GX_End.
//
// Pass 1 finds the runs (so we know how many vertices to
// announce), pass 2 draws them.
// ============================================================

typedef struct
{
    u8 row;
    u8 start;
    u8 len;
} GlyphRun;


static void drawGlyph(
    const unsigned char *glyph,
    f32 x,
    f32 y,
    f32 scale,
    u8 r,
    u8 g,
    u8 b
)
{
    GlyphRun runs[7 * 3];   // at most 3 runs per row (pattern 10101)
    int      count = 0;
    int      row;
    int      col;
    int      i;

    // ---- Pass 1: find the runs ----
    for (row = 0; row < 7; row++)
    {
        unsigned char bits = glyph[row];

        col = 0;

        while (col < 5)
        {
            if (bits & (1 << (4 - col)))
            {
                int start = col;

                while (col < 5 && (bits & (1 << (4 - col))))
                    col++;

                runs[count].row   = (u8)row;
                runs[count].start = (u8)start;
                runs[count].len   = (u8)(col - start);
                count++;
            }
            else
            {
                col++;
            }
        }
    }

    if (count == 0)
        return;     // e.g. the space character

    // ---- Pass 2: draw them ----
    GX_Begin(GX_QUADS, GX_VTXFMT0, count * 4);

        for (i = 0; i < count; i++)
        {
            f32 px = x + ((f32)runs[i].start * scale);
            f32 py = y + ((f32)runs[i].row   * scale);

            emitQuad(
                px,
                py,
                px + ((f32)runs[i].len * scale),
                py + scale,
                r,
                g,
                b
            );
        }

    GX_End();
}


// ============================================================
// Draw a single character
//
// Returns the width consumed by the character.
//
// The font itself is 5 pixels wide.
// We add one pixel of spacing.
//
// Therefore:
//
//     character width = 6 * scale
//
// ============================================================

static f32 drawChar(
    char c,
    f32 x,
    f32 y,
    f32 scale,
    u8 r,
    u8 g,
    u8 b
)
{
    const unsigned char *glyph = getGlyph(c);

    if (glyph != NULL)
    {
        drawGlyph(
            glyph,
            x,
            y,
            scale,
            r,
            g,
            b
        );
    }

    return 6.0f * scale;
}


// ============================================================
// Draw a complete string
//
// Example:
//
//     drawText(
//         "Hello World!",
//         220.0f,
//         120.0f,
//         4.0f,
//         255,
//         255,
//         255
//     );
//
// ============================================================

static void drawText(
    const char *text,
    f32 x,
    f32 y,
    f32 scale,
    u8 r,
    u8 g,
    u8 b
)
{
    f32 cursorX = x;

    if (text == NULL)
        return;

    while (*text != '\0')
    {
        cursorX += drawChar(
            *text,
            cursorX,
            y,
            scale,
            r,
            g,
            b
        );

        text++;
    }
}


// ============================================================
// Turn one working value into text
//
//     floats  ->  "80.00", "0.15", "-1.00"
//     ints    ->  "5"
//     bools   ->  "ON" / "OFF"
//     style   ->  "0 TROPICAL"
//
// Floats are printed with integer maths on purpose, so this
// does not depend on printf float support.
// ============================================================

static void formatValue(int index, char *out, size_t size)
{
    const DebugItem *item  = &s_items[index];
    float            value = s_work[index];

    switch (item->type)
    {
        case DEBUG_FLOAT:
        {
            bool negative   = (value < 0.0f);
            int  hundredths = (int)((negative ? -value : value) * 100.0f + 0.5f);

            snprintf(
                out,
                size,
                "%s%d.%02d",
                (negative && hundredths != 0) ? "-" : "",
                hundredths / 100,
                hundredths % 100
            );
            break;
        }

        case DEBUG_BOOL:
            snprintf(out, size, "%s", (value > 0.5f) ? "ON" : "OFF");
            break;

        case DEBUG_STYLE:
        {
            int         style = (int)(value + 0.5f);
            const char *name  = "?";

            if (style >= 0 && style < NUM_ISLAND_STYLES && s_styleNames[style] != NULL)
                name = s_styleNames[style];

            snprintf(out, size, "%d %s", style, name);
            break;
        }

        default:    // DEBUG_INT
            snprintf(out, size, "%d", (int)(value + 0.5f));
            break;
    }
}


// ============================================================
// Initialize
// ============================================================

void initDebugMenu(void)
{
    s_open           = false;
    s_applyRequested = false;
    s_selected       = 0;

    s_holdUp    = 0;
    s_holdDown  = 0;
    s_holdLeft  = 0;
    s_holdRight = 0;

    loadWorkingCopy();
}


// ============================================================
// Public state queries (used by main.c)
// ============================================================

bool debugMenuIsOpen(void)
{
    return s_open;
}


bool debugMenuConsumeApply(void)
{
    bool requested   = s_applyRequested;
    s_applyRequested = false;
    return requested;
}


// ============================================================
// Update
//
// Call once per frame, right after PAD_ScanPads().
// ============================================================

void updateDebugMenu(void)
{
    u32 down = PAD_ButtonsDown(0);
    u32 held = PAD_ButtonsHeld(0);
    int scale;

    // --------------------------------------------------------
    // Closed: the only thing we listen for is D-pad UP.
    // --------------------------------------------------------

    if (!s_open)
    {
        if (down & PAD_BUTTON_UP)
        {
            loadWorkingCopy();      // start from the current real values
            s_open = true;

            s_holdUp    = 0;
            s_holdDown  = 0;
            s_holdLeft  = 0;
            s_holdRight = 0;
        }

        return;
    }

    // --------------------------------------------------------
    // X: close and forget the edits.
    // (The real variables were never touched.)
    // --------------------------------------------------------

    if (down & PAD_BUTTON_X)
    {
        s_open = false;
        return;
    }

    // --------------------------------------------------------
    // Y: close and apply the edits.
    // main.c sees debugMenuConsumeApply() and regenerates.
    // --------------------------------------------------------

    if (down & PAD_BUTTON_Y)
    {
        applyWorkingCopy();
        s_applyRequested = true;
        s_open           = false;
        return;
    }

    // --------------------------------------------------------
    // UP / DOWN: move the highlight (wraps around).
    // --------------------------------------------------------

    if (buttonRepeat(PAD_BUTTON_UP, &s_holdUp))
        s_selected = (s_selected + ITEM_COUNT - 1) % ITEM_COUNT;

    if (buttonRepeat(PAD_BUTTON_DOWN, &s_holdDown))
        s_selected = (s_selected + 1) % ITEM_COUNT;

    // --------------------------------------------------------
    // LEFT / RIGHT: change the highlighted value.
    //
    // Hold R for bigger steps (x10), L for even bigger (x100).
    // --------------------------------------------------------

    scale = 1;

    if (held & PAD_TRIGGER_R) scale = 10;
    if (held & PAD_TRIGGER_L) scale = 100;

    if (buttonRepeat(PAD_BUTTON_LEFT, &s_holdLeft))
        adjustItem(s_selected, -1, scale);

    if (buttonRepeat(PAD_BUTTON_RIGHT, &s_holdRight))
        adjustItem(s_selected, +1, scale);
}


// ============================================================
// Menu layout (screen is 640 x 480)
// ============================================================

#define PANEL_X1     60.0f
#define PANEL_Y1     20.0f
#define PANEL_X2    580.0f
#define PANEL_Y2    456.0f

#define LABEL_X      80.0f
#define VALUE_X     360.0f

#define ROW_TOP      66.0f
#define ROW_HEIGHT   16.0f      // 20 rows must end above the help text at y=402

#define TEXT_SCALE    2.0f      // 5x7 font -> 10x14 pixels per character


// ============================================================
// Draw
// ============================================================

void drawDebugMenu(void)
{
    char buffer[32];
    int  i;

    if (!s_open)
        return;


    // --------------------------------------------------------
    // Set up a simple 640x480 orthographic projection.
    //
    // Coordinates:
    //
    //     X = 0 -> 640
    //     Y = 0 -> 480
    //
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
    // With no depth test, things drawn LATER appear on top, so
    // the order below matters: panel, highlight bar, then text.
    // --------------------------------------------------------

    GX_SetZMode(
        GX_FALSE,
        GX_ALWAYS,
        GX_FALSE
    );


    // --------------------------------------------------------
    // Panel: grey border, black fill.
    // --------------------------------------------------------

    drawRect2D(
        PANEL_X1 - 2.0f,
        PANEL_Y1 - 2.0f,
        PANEL_X2 + 2.0f,
        PANEL_Y2 + 2.0f,
        90,
        90,
        90
    );

    drawRect2D(
        PANEL_X1,
        PANEL_Y1,
        PANEL_X2,
        PANEL_Y2,
        0,
        0,
        0
    );

    drawText(
        "DEBUG MENU",
        LABEL_X,
        32.0f,
        3.0f,
        255,
        255,
        255
    );


    // --------------------------------------------------------
    // One row per item.
    //
    //     selected row  -> blue bar, yellow text
    //     other rows    -> light grey text
    //     edited value  -> green (differs from the real value)
    // --------------------------------------------------------

    for (i = 0; i < ITEM_COUNT; i++)
    {
        f32  y        = ROW_TOP + ((f32)i * ROW_HEIGHT);
        bool selected = (i == s_selected);
        bool changed  = (s_work[i] != readTarget(&s_items[i]));

        u8 labelR = selected ? 255 : 210;
        u8 labelG = selected ? 255 : 210;
        u8 labelB = selected ?   0 : 210;

        u8 valueR = changed ?  90 : labelR;
        u8 valueG = changed ? 255 : labelG;
        u8 valueB = changed ?  90 : labelB;

        if (selected)
        {
            drawRect2D(
                PANEL_X1 + 8.0f,
                y - 1.0f,
                PANEL_X2 - 8.0f,
                y + 15.0f,
                0,
                0,
                110
            );
        }

        drawText(
            s_items[i].label,
            LABEL_X,
            y,
            TEXT_SCALE,
            labelR,
            labelG,
            labelB
        );

        formatValue(i, buffer, sizeof(buffer));

        drawText(
            buffer,
            VALUE_X,
            y,
            TEXT_SCALE,
            valueR,
            valueG,
            valueB
        );
    }


    // --------------------------------------------------------
    // Help text.
    // --------------------------------------------------------

    drawText(
        "UP/DOWN: SELECT  LEFT/RIGHT: CHANGE",
        LABEL_X,
        402.0f,
        TEXT_SCALE,
        170,
        170,
        170
    );

    drawText(
        "R: x10  L: x100  X: CANCEL  Y: APPLY",
        LABEL_X,
        422.0f,
        TEXT_SCALE,
        170,
        170,
        170
    );


    // --------------------------------------------------------
    // Restore normal depth testing.
    // --------------------------------------------------------

    GX_SetZMode(
        GX_TRUE,
        GX_LEQUAL,
        GX_TRUE
    );
}