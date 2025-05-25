#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <gccore.h>
#include "gx_utils.h"
#include <stdbool.h>
#include "common.h"
#include "water.h"
#include "island.h"
#include "boat.h"
#include "manager.h"

int main(int argc, char** argv) {
    // Original main function code exactly as you wrote it
    f32 yscale;
    u32 xfbHeight;
    Mtx view;
    Mtx44 perspective;
    Mtx model, modelview;
    u32 fb = 0;
    GXColor background = { 135, 206, 255, 255 };


    // Initialize the VI and WPAD.
    VIDEO_Init();
    PAD_Init();

    IslandManager islandManager;
    initIslandManager(&islandManager);

    // Create some islands
    createIsland(&islandManager, 0.0f, -15.0f, ISLAND_RADIUS);
    createIsland(&islandManager, 20.0f, 10.0f, ISLAND_RADIUS);
    createIsland(&islandManager, -15.0f, 20.0f, ISLAND_RADIUS);

    rmode = VIDEO_GetPreferredMode(NULL);

    // Allocate 2 framebuffers for double buffering
    frameBuffer[0] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    frameBuffer[1] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(frameBuffer[fb]);
    VIDEO_SetBlack(false);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if (rmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();

    // Setup the FIFO and initialize the Flipper
    void* gp_fifo = memalign(32, DEFAULT_FIFO_SIZE);
    memset(gp_fifo, 0, DEFAULT_FIFO_SIZE);
    GX_Init(gp_fifo, DEFAULT_FIFO_SIZE);

    // Clear the background and the Z buffer
    GX_SetCopyClear(background, 0x00ffffff);

    // Other GX setup
    GX_SetViewport(0, 0, rmode->fbWidth, rmode->efbHeight, 0, 1);
    yscale = GX_GetYScaleFactor(rmode->efbHeight, rmode->xfbHeight);
    xfbHeight = GX_SetDispCopyYScale(yscale);
    GX_SetScissor(0, 0, rmode->fbWidth, rmode->efbHeight);
    GX_SetDispCopySrc(0, 0, rmode->fbWidth, rmode->efbHeight);
    GX_SetDispCopyDst(rmode->fbWidth, xfbHeight);
    GX_SetCopyFilter(rmode->aa, rmode->sample_pattern, GX_TRUE, rmode->vfilter);
    GX_SetFieldMode(rmode->field_rendering, ((rmode->viHeight == 2 * rmode->xfbHeight) ? GX_ENABLE : GX_DISABLE));

    GX_SetCullMode(GX_CULL_NONE);
    GX_CopyDisp(frameBuffer[fb], GX_TRUE);
    GX_SetDispCopyGamma(GX_GM_1_0);

    // Setup the vertex descriptor
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);

    // Setup the vertex attribute table
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGB8, 0);

    GX_SetNumChans(1);
    GX_SetNumTexGens(0);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);

    // Boat and camera setup
    guVector boatPos = { 0.0f, 0.0f, 0.0f },  // Boat position
        cam = { 0.0F, 0.5F, 5.0F },  // Camera position (behind and above boat)
        up = { 0.0F, 1.0F, 0.0F },
        look = { 0.0F, 0.0F, 0.0F };  // Camera looks at the boat
    float boatYaw = 0.0f; // Boat's yaw (rotation around y-axis)
    f32 boatSpeed = 0.2f; // Speed of boat movement

    // Setup the initial view matrix
    guLookAt(view, &cam, &up, &look);

    // Setup our projection matrix (Perspective)
    f32 w = rmode->viWidth;
    f32 h = rmode->viHeight;
    guPerspective(perspective, 45, (f32)w / h, 0.1F, 100.0F);  // Adjust far clipping distance
    GX_LoadProjectionMtx(perspective, GX_PERSPECTIVE);

    // Time variable for animation
    f32 time = 0.0f;

    // Main game loop
    while (1) {
        PAD_ScanPads();

        if (PAD_ButtonsDown(0) & PAD_BUTTON_START) exit(0);

        // Add this block to handle A button press
        if (PAD_ButtonsDown(0) & PAD_BUTTON_A) {
            regenerateIslands(&islandManager);
        }

        const float threshold = 2.0f;

        bool left = false, right = false, upp = false, down = false;
        float joystickX = PAD_StickX(0);
        float joystickY = PAD_StickY(0);

        if (joystickX < -threshold) left = true;   // Left
        if (joystickX > threshold) right = true;   // Right
        if (joystickY < -threshold) down = true;   // Down
        if (joystickY > threshold) upp = true;      // Up

        // Boat movement using Wii remote buttons
        // In the boat movement section, replace with:
        if (upp) { // Move boat forward
            float boatHeight = sinf((boatPos.x + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE +
                cosf((boatPos.z + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE;

            Vec3 newPos = {
                boatPos.x - sinf(boatYaw) * boatSpeed,
                boatHeight,
                boatPos.z + cosf(boatYaw) * boatSpeed
            };

            if (!checkAllIslandsCollision(&islandManager, newPos, cRadius)) {
                boatPos.x = newPos.x;
                boatPos.z = newPos.z;
            }

            
        }

        if (down) { // Move boat backward
            float boatHeight = sinf((boatPos.x + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE +
                cosf((boatPos.z + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE;

            Vec3 newPos = {
                boatPos.x + sinf(boatYaw) * boatSpeed,
                boatHeight,
                boatPos.z - cosf(boatYaw) * boatSpeed
            };

            if (!checkAllIslandsCollision(&islandManager, newPos, cRadius)) {
                
            }
            boatPos.x = newPos.x;
            boatPos.z = newPos.z;
        }

        if (left) { // Turn boat left
            boatYaw -= 0.05f;
        }

        if (right) { // Turn boat right
            boatYaw += 0.05f;
        }

        // Camera follows the boat
        cam.x = boatPos.x + sinf(boatYaw) * 5.0f;  // Camera stays behind the boat
        cam.z = boatPos.z - cosf(boatYaw) * 5.0f;

        // Update the camera's look vector based on the boat's position and yaw
        look.x = boatPos.x + sinf(boatYaw) * 2.0f;
        look.y = boatPos.y;
        look.z = boatPos.z - cosf(boatYaw) * 2.0f;

        // Recalculate the view matrix with the updated look-at point
        guLookAt(view, &cam, &up, &look);




        int numIter = 5;
        if (time >= numIter * (2 * M_PI / WAVE_FREQUENCY)) {
            time -= numIter * (2 * M_PI / WAVE_FREQUENCY);
            time -= (1 / 2) * (WAVE_SPEED * WAVE_FREQUENCY);
        }


        // Set viewport
        GX_SetViewport(0, 0, rmode->fbWidth, rmode->efbHeight, 0, 1);

        // Create a simple wave effect on the water
        guMtxIdentity(model);
        guMtxTransApply(model, model, 0.0f, -1.0f, 0.0f);  // Position water below the camera
        guMtxConcat(view, model, modelview);
        GX_LoadPosMtxImm(modelview, GX_PNMTX0);

        drawWater(time);

        // Increment time for wave movement
        time += WAVE_SPEED;
        // Resets time variable so no overflow

        // Draw the island (spherical shape)
        drawAllIslands(&islandManager);  // Place island at the origin

        // Draw the boat in front of the player
        float boatHeight = sinf((boatPos.x + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE +
            cosf((boatPos.z + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE;
        drawBoat(boatPos.x, boatHeight, boatPos.z, boatYaw);  // Boat position and yaw

        // Finalize drawing
        GX_DrawDone();

        // Swap framebuffers for double buffering
        fb ^= 1;
        GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
        GX_SetColorUpdate(GX_TRUE);
        GX_CopyDisp(frameBuffer[fb], GX_TRUE);

        VIDEO_SetNextFramebuffer(frameBuffer[fb]);
        VIDEO_Flush();
        VIDEO_WaitVSync();
    }

    freeAllIslands(&islandManager);
    return 0;
}