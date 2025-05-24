#include "gx_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <stdbool.h>

void* frameBuffer[2] = { NULL, NULL };
GXRModeObj* rmode = NULL;

void init_graphics() {
    VIDEO_Init();
    rmode = VIDEO_GetPreferredMode(NULL);

    frameBuffer[0] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    frameBuffer[1] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(frameBuffer[0]);
    VIDEO_SetBlack(false);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if (rmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();
}

void setup_camera(guVector cam, guVector up, guVector look) {
    Mtx view;
    Mtx44 perspective;

    guLookAt(view, &cam, &up, &look);
    guPerspective(perspective, 45, (f32)rmode->viWidth / rmode->viHeight, 0.1F, 100.0F);

    GX_LoadProjectionMtx(perspective, GX_PERSPECTIVE);
    GX_LoadPosMtxImm(view, GX_PNMTX0);
}

void begin_frame() {
    GX_SetViewport(0, 0, rmode->fbWidth, rmode->efbHeight, 0, 1);
    GX_SetScissor(0, 0, rmode->fbWidth, rmode->efbHeight);
    GX_SetCopyClear((GXColor) { 135, 206, 255, 255 }, 0x00ffffff);
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