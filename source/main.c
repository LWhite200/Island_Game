// main.c
// Entry point and main game loop.
//
// This is deliberately bare-bones: sail a boat around an ocean full of
// hemisphere islands, or hop out and walk around on foot. That's it --
// no enemies, no collectibles, no HUD. Just the core loop, so it's easy
// to see exactly what's going on before building anything back on top.
//
// Controls
// ------------------------------------------------------------
//  Left stick         - move / steer
//  C-stick left/right - orbit camera around the active entity
//  C-stick up/down    - raise / lower camera angle
//  A button           - regenerate the world (debug)
//  B button           - board / disembark the boat
//  Z / R button       - jump (on foot)
//  START              - quit

#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <gccore.h>
#include "gx_utils.h"
#include "common.h"
#include "water.h"
#include "island.h"
#include "boat.h"
#include "player.h"
#include "camera.h"
#include "boundary.h"
#include "debug.h"

#define GP_FIFO_SIZE (256 * 1024)

int main(void) {
    // ---- Hardware init ----
    PAD_Init();
    void* gpFifo = memalign(32, GP_FIFO_SIZE);
    init_graphics(gpFifo, GP_FIFO_SIZE); // video + GX pipeline, see gx_utils.c

    // ---- World / entities ----
    IslandManager world;
    initIslandManager(&world);
    regenerateIslands(&world);

    Boat   boat;
    Player player;
    Camera camera;
    initBoat(&boat);
    initPlayer(&player);
    initCamera(&camera);
    initDebugMenu();

    bool isPlayerActive = true; // false = sailing the boat, true = on foot

    u32 fb   = 0;
    f32 time = 0.0f;

    // ============================================================
    // Main loop
    // ============================================================

    while (1) {
        PAD_ScanPads();

        if (PAD_ButtonsDown(0) & PAD_BUTTON_START) exit(0);

        // Handle Debug menu inputs
        updateDebugMenu();

        // ---- C-stick (camera orbit) ----
        const float CSTICK_DEAD = 8.0f;
        float cStickX = (float)PAD_SubStickX(0);
        float cStickY = (float)PAD_SubStickY(0);
        float cYaw    = (fabsf(cStickX) > CSTICK_DEAD) ? cStickX / 80.0f : 0.0f;
        float cPitch  = (fabsf(cStickY) > CSTICK_DEAD) ? cStickY / 80.0f : 0.0f;

        // ---- Main stick (movement) ----
        const float DEAD_ZONE = 2.0f;
        float jx = (float)PAD_StickX(0);
        float jy = (float)PAD_StickY(0);
        bool moveFwd   = jy >  DEAD_ZONE;
        bool moveBack  = jy < -DEAD_ZONE;
        bool moveLeft  = jx < -DEAD_ZONE;
        bool moveRight = jx >  DEAD_ZONE;

        // ---- Buttons ----
        bool btnA = (PAD_ButtonsDown(0) & PAD_BUTTON_A)  != 0;
        bool btnB = (PAD_ButtonsDown(0) & PAD_BUTTON_B)  != 0;
        bool btnZ = (PAD_ButtonsDown(0) & PAD_TRIGGER_Z) != 0;
        bool btnR = (PAD_ButtonsDown(0) & PAD_TRIGGER_R) != 0;
        bool jump = (btnZ || btnR) && isPlayerActive;

        // ---- A: regenerate world (debug helper, works in either mode) ----
        if (btnA) {
            freeAllIslands(&world);
            regenerateIslands(&world);
        }

        // ---- B: board / disembark ----
        /*
        if (btnB) {
            if (!isPlayerActive) {
                // Disembark: only when the boat is touching land
                Vec3 boatPos = { boat.position.x, boat.position.y, boat.position.z };
                if (checkAllIslandsCollision(&world, boatPos, boat.radius)) {
                    isPlayerActive   = true;
                    player.position  = boat.position;
                    player.yaw       = boat.yaw;
                    player.yVelocity = 0.0f;
                }
            } else {
                // Re-board: only near sea level
                if (player.position.y <= boatChangeY) {
                    boat.position.x = player.position.x;
                    boat.position.z = player.position.z;
                    boat.position.y = 0.0f;
                    boat.yaw        = player.yaw;
                    isPlayerActive  = false;
                    initCamera(&camera);
                }
            }
        }
        */

        

        // ---- Jump: only while standing on something ----
        if (jump && playerIsGrounded(&player))
            player.yVelocity = JUMP_FORCE;

        // ---- figure out current player location ----
        Vec3 trackPos = isPlayerActive
            ? (Vec3){ player.position.x, player.position.y, player.position.z }
            : (Vec3){ boat.position.x,   boat.position.y,   boat.position.z   };

        // ---- Update the active entity ----
        if (isPlayerActive)
            updatePlayer(&player, moveFwd, moveBack, moveLeft, moveRight, &world);
        else
            updateBoat(&boat, moveFwd, moveBack, moveLeft, moveRight, time, &world);

        updateCamera(&camera, &boat, &player, isPlayerActive, &world, cYaw, cPitch);

        // ---- Advance wave time (and wrap so it never grows unbounded) ----
        time += WAVE_SPEED;

        // ============================================================
        // Rendering
        // ============================================================

        begin_frame();
        setup_camera(camera.position, camera.up, camera.look);

        drawWater(time, trackPos.x, trackPos.z);
        drawAllIslands(&world, trackPos, 1.5f); // (or whatever radius fits your player/boat size)
        drawWorldBoundary();

        if (isPlayerActive)
            drawPlayer(player.position.x, player.position.y, player.position.z, player.yaw);
        else
            drawBoat(boat.position.x, boat.position.y, boat.position.z, boat.yaw);


        drawDebugMenu();

        end_frame(fb);
        fb ^= 1;
    }

    return 0;
}
