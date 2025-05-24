#ifndef GX_UTILS_H
#define GX_UTILS_H

#include <gccore.h>
#include "common.h"

extern void* frameBuffer[2];
extern GXRModeObj* rmode;  // Change this to extern

void init_graphics();
void setup_camera(guVector cam, guVector up, guVector look);
void begin_frame();
void end_frame();

#endif