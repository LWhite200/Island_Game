// gx_utils.h
//
// Everything needed to get the Wii's video + GX (graphics) hardware ready
// to draw, tucked away here so main.c can just call one function and get
// on with the actual game.

#pragma once
#include <gccore.h>

extern void*       frameBuffer[2]; // The two framebuffers we flip between
extern GXRModeObj*  rmode;         // The video mode the Wii picked for us

// One-time setup: video mode, double-buffered framebuffers, and the whole
// GX pipeline (vertex format, viewport, clear colour). Call this once at
// the very start of main(), before the game loop.
//   gpFifo / fifoSize -- a block of memory for GX's command queue.
void init_graphics(void* gpFifo, u32 fifoSize);

// Builds a perspective projection + look-at view matrix from a camera
// position/up/look and loads both into GX. Call once per frame.
void setup_camera(guVector cam, guVector up, guVector look);

// Call at the start of every frame, before drawing anything.
void begin_frame(void);

// Call at the end of every frame. `fb` is which framebuffer (0 or 1) to
// present next -- the caller is responsible for flipping it between calls.
void end_frame(int fb);
