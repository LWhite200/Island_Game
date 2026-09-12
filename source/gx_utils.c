// gx_utils.c
//
// Thin wrappers around the VIDEO + GX initialisation sequence, so main.c
// doesn't have to be cluttered with hardware setup boilerplate. This is
// all standard devkitPPC/libogc setup -- nothing here is game logic.

#include "gx_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <stdbool.h>

// Global framebuffer pointers -- indexed by the double-buffer toggle (0 or 1)
void*      frameBuffer[2] = { NULL, NULL };
GXRModeObj* rmode         = NULL;


// ============================================================
// Initialisation
// ============================================================

void init_graphics(void* gpFifo, u32 fifoSize) {
    // ---- Video + framebuffers ----
    VIDEO_Init();
    rmode = VIDEO_GetPreferredMode(NULL);

    // Allocate framebuffers in the uncached "K1" region so the GPU can write them
    frameBuffer[0] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    frameBuffer[1] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(frameBuffer[0]);
    VIDEO_SetBlack(false);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if (rmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();

    // ---- GX (the actual 3-D graphics chip) ----
    memset(gpFifo, 0, fifoSize);
    GX_Init(gpFifo, fifoSize);

    GX_SetCopyClear((GXColor){ 135, 206, 255, 255 }, 0x00ffffff); // sky blue

    GX_SetViewport(0, 0, rmode->fbWidth, rmode->efbHeight, 0, 1);
    f32 yscale    = GX_GetYScaleFactor(rmode->efbHeight, rmode->xfbHeight);
    u32 xfbHeight = GX_SetDispCopyYScale(yscale);
    GX_SetScissor(0, 0, rmode->fbWidth, rmode->efbHeight);
    GX_SetDispCopySrc(0, 0, rmode->fbWidth, rmode->efbHeight);
    GX_SetDispCopyDst(rmode->fbWidth, xfbHeight);
    GX_SetCopyFilter(rmode->aa, rmode->sample_pattern, GX_TRUE, rmode->vfilter);
    GX_SetFieldMode(rmode->field_rendering,
                    (rmode->viHeight == 2 * rmode->xfbHeight) ? GX_ENABLE : GX_DISABLE);
    GX_SetCullMode(GX_CULL_NONE);
    GX_CopyDisp(frameBuffer[0], GX_TRUE);
    GX_SetDispCopyGamma(GX_GM_1_0);

    // Every vertex we ever draw has just a position and a colour -- no
    // textures, no lighting. Simple on purpose.
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS,  GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS,  GX_POS_XYZ,  GX_F32,  0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGB8, 0);
    GX_SetNumChans(1);
    GX_SetNumTexGens(0);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
}


// ============================================================
// Per-frame camera
// ============================================================

// Loads a perspective projection and a look-at view matrix into GX slot 0.
// Every object in the game is drawn in plain world-space coordinates (there
// is no separate "model" matrix anywhere), so the view matrix is all we
// ever need to load.
void setup_camera(guVector cam, guVector up, guVector look) {
    Mtx   view;
    Mtx44 proj;

    guLookAt(view, &cam, &up, &look);
    guPerspective(proj, 45.0f, (f32)rmode->viWidth / rmode->viHeight, 0.1f, 300.0f);

    GX_LoadProjectionMtx(proj,  GX_PERSPECTIVE);
    GX_LoadPosMtxImm(view, GX_PNMTX0);
}


// ============================================================
// Frame begin / end
// ============================================================

void begin_frame(void) {
    GX_SetViewport(0, 0, rmode->fbWidth, rmode->efbHeight, 0, 1);
    GX_SetScissor(0, 0, rmode->fbWidth, rmode->efbHeight);
}

void end_frame(int fb) {
    GX_DrawDone();
    GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GX_SetColorUpdate(GX_TRUE);
    GX_CopyDisp(frameBuffer[fb], GX_TRUE);

    VIDEO_SetNextFramebuffer(frameBuffer[fb]);
    VIDEO_Flush();
    VIDEO_WaitVSync();
}
