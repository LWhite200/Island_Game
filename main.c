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
#include "player.h"
#include "manager.h"

// Boat structure to hold boat-related data
typedef struct {
    guVector position;
    float yaw;
    float speed;
    float radius;
} Boat;

// Player structure to hold player-related data
typedef struct {
    guVector position;
    float yaw;
    float speed;
    float radius;
} Player;

// Camera structure to hold camera-related data
typedef struct {
    guVector position;
    guVector up;
    guVector look;
    float followDistance;
    float heightOffset;
} Camera;

// Initialize boat with default values
void initBoat(Boat* boat) {
    boat->position.x = 0.0f;
    boat->position.y = 0.0f;
    boat->position.z = 0.0f;
    boat->yaw = 0.0f;
    boat->speed = 0.2f;
    boat->radius = 1.0f;
}

// Initialize player with default values
void initPlayer(Player* player) {
    player->position.x = 0.0f;
    player->position.y = 0.0f;
    player->position.z = 0.0f;
    player->yaw = 0.0f;
    player->speed = 0.2f;
    player->radius = 1.0f;
}

// Initialize camera with default values
void initCamera(Camera* camera) {
    camera->position.x = 0.0f;
    camera->position.y = 0.5f;
    camera->position.z = 5.0f;
    camera->up.x = 0.0f;
    camera->up.y = 1.0f;
    camera->up.z = 0.0f;
    camera->look.x = 0.0f;
    camera->look.y = 0.0f;
    camera->look.z = 0.0f;
    camera->followDistance = 5.0f;
    camera->heightOffset = 0.5f;
}

void drawIndicator(Vec3 position) {
    float yOffset = 0.5f;         // Height above the position
    float size = 0.5f;            // Size of the triangle

    // Triangle will float above the position
    float x = position.x;
    float y = position.y + yOffset;
    float z = position.z;

    // Define 3 vertices of a triangle centered above the point
    // These form a flat triangle pointing up in the XZ plane
    GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 3);

    // Top vertex
    GX_Position3f32(x, y + size, z);
    GX_Color3f32(1.0f, 0.0f, 0.0f);  

    // Bottom-left vertex
    GX_Position3f32(x - size, y, z);
    GX_Color3f32(0.0f, 1.0f, 0.0f);  

    // Bottom-right vertex
    GX_Position3f32(x + size, y, z);
    GX_Color3f32(0.0f, 0.0f, 1.0f); 

    GX_End();
}


// Update boat position based on input
// Update boat position based on input
void updateBoat(Boat* boat, bool upp, bool down, bool left, bool right, float time, IslandManager* islandManager) {

    // Compute wave height at boat's current position
    float boatHeight = sinf((boat->position.x + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE +
        cosf((boat->position.z + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE;

    // Current position of the boat (for collision and indicator)
    Vec3 curPos = {
        boat->position.x,
        boatHeight,
        boat->position.z
    };

    // Forward and backward position for collision checking
    Vec3 backwardPos = {
        boat->position.x + sinf(boat->yaw) * boat->speed,
        boatHeight,
        boat->position.z - cosf(boat->yaw) * boat->speed
    };

    Vec3 forwardPos = {
        boat->position.x - sinf(boat->yaw) * boat->speed,
        boatHeight,
        boat->position.z + cosf(boat->yaw) * boat->speed
    };

    // Collision checks
    bool frontBlocked = checkAllIslandsCollision(islandManager, forwardPos, boat->radius);
    bool currentlyBlocked = checkAllIslandsCollision(islandManager, curPos, boat->radius);
    bool behindBlocked = checkAllIslandsCollision(islandManager, backwardPos, boat->radius);

    // Show indicator if near island
    if (frontBlocked || currentlyBlocked || behindBlocked) {
        drawIndicator(curPos);
    }

    // Movement
    if (down && !behindBlocked) {
        boat->position.x += sinf(boat->yaw) * boat->speed;
        boat->position.z -= cosf(boat->yaw) * boat->speed;
    }

    if (upp && !frontBlocked) {
        boat->position.x -= sinf(boat->yaw) * boat->speed;
        boat->position.z += cosf(boat->yaw) * boat->speed;
    }

    // Rotation
    if (left) {
        boat->yaw -= 0.05f;
    }

    if (right) {
        boat->yaw += 0.05f;
    }
}











// Update player position based on input (same as boat for now)
void updatePlayer(Player* player, bool upp, bool down, bool left, bool right, float time, IslandManager* islandManager) {
    if (upp) { // Move player forward
        float playerHeight = sinf((player->position.x + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE +
            cosf((player->position.z + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE;

        Vec3 newPos = {
            player->position.x - sinf(player->yaw) * player->speed,
            playerHeight,
            player->position.z + cosf(player->yaw) * player->speed
        };

        if (!checkAllIslandsCollision(islandManager, newPos, player->radius)) {
            player->position.x = newPos.x;
            player->position.z = newPos.z;
        }
    }

    if (down) { // Move player backward
        float playerHeight = sinf((player->position.x + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE +
            cosf((player->position.z + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE;

        Vec3 newPos = {
            player->position.x + sinf(player->yaw) * player->speed,
            playerHeight,
            player->position.z - cosf(player->yaw) * player->speed
        };

        if (!checkAllIslandsCollision(islandManager, newPos, player->radius)) {
            player->position.x = newPos.x;
            player->position.z = newPos.z;
        }
    }

    if (left) { // Turn player left
        player->yaw -= 0.05f;
    }

    if (right) { // Turn player right
        player->yaw += 0.05f;
    }
}

// Update camera to follow either boat or player
void updateCamera(Camera* camera, const Boat* boat, const Player* player, bool isPlayerActive) {
    if (isPlayerActive) {
        // Camera follows player
        camera->position.x = player->position.x + sinf(player->yaw) * camera->followDistance;
        camera->position.z = player->position.z - cosf(player->yaw) * camera->followDistance;

        // Update the camera's look vector
        camera->look.x = player->position.x + sinf(player->yaw) * 2.0f;
        camera->look.y = player->position.y;
        camera->look.z = player->position.z - cosf(player->yaw) * 2.0f;
    }
    else {
        // Camera follows boat
        camera->position.x = boat->position.x + sinf(boat->yaw) * camera->followDistance;
        camera->position.z = boat->position.z - cosf(boat->yaw) * camera->followDistance;

        // Update the camera's look vector
        camera->look.x = boat->position.x + sinf(boat->yaw) * 2.0f;
        camera->look.y = boat->position.y;
        camera->look.z = boat->position.z - cosf(boat->yaw) * 2.0f;
    }
}

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
    createIsland(&islandManager, 0.0f, -15.0f);
    createIsland(&islandManager, 20.0f, 10.0f);
    createIsland(&islandManager, -15.0f, 20.0f);

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

    // Boat, player and camera setup
    Boat boat;
    Player player;
    Camera camera;
    initBoat(&boat);
    initPlayer(&player);
    initCamera(&camera);

    bool isPlayerActive = false; // Start with boat active

    // Setup the initial view matrix
    guLookAt(view, &camera.position, &camera.up, &camera.look);

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

        // Handle B button press (switch between boat and player)
        // Handle B button press (switch between boat and player)
        if (PAD_ButtonsDown(0) & PAD_BUTTON_B) {
            // When switching to player, check if boat is on land first
            if (!isPlayerActive) {
                Vec3 boatPos = {
                    boat.position.x,
                    sinf((boat.position.x + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE +
                    cosf((boat.position.z + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE,
                    boat.position.z
                };

                if (checkAllIslandsCollision(&islandManager, boatPos, boat.radius)) {
                    // Boat is on land, allow switching to player
                    isPlayerActive = true;
                    player.position = boat.position;
                    player.yaw = boat.yaw;
                }
            }
            else {
                // Switching back to boat from player
                isPlayerActive = false;
                boat.position = player.position;
                boat.yaw = player.yaw;
            }
        }

        const float threshold = 2.0f;

        bool left = false, right = false, upp = false, down = false;
        float joystickX = PAD_StickX(0);
        float joystickY = PAD_StickY(0);

        if (joystickX < -threshold) left = true;   // Left
        if (joystickX > threshold) right = true;   // Right
        if (joystickY < -threshold) down = true;   // Down
        if (joystickY > threshold) upp = true;      // Up

        // Update boat and camera
        // Update either boat or player based on current mode
        if (isPlayerActive) {
            updatePlayer(&player, upp, down, left, right, time, &islandManager);
        }
        else {
            updateBoat(&boat, upp, down, left, right, time, &islandManager);
        }
        // Update camera to follow the active entity
        updateCamera(&camera, &boat, &player, isPlayerActive);

        // Recalculate the view matrix with the updated look-at point
        guLookAt(view, &camera.position, &camera.up, &camera.look);




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

        // drawWater(time);

        // Increment time for wave movement
        time += WAVE_SPEED;
        // Resets time variable so no overflow

        // Draw the island (spherical shape)
        drawAllIslands(&islandManager);  // Place island at the origin

        if (isPlayerActive) {
            drawIslandHitArea(&islandManager, (Vec3) { player.position.x, player.position.y, player.position.z }, player.radius);
        }

        // Draw the boat in front of the player
        // Draw both boat and player, but only show the active one
        if (isPlayerActive) {
            float playerHeight = sinf((player.position.x + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE +
                cosf((player.position.z + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE;
            drawPlayer(player.position.x, playerHeight, player.position.z, player.yaw);
        }
        else {
            float boatHeight = sinf((boat.position.x + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE +
                cosf((boat.position.z + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE;
            drawBoat(boat.position.x, boatHeight, boat.position.z, boat.yaw);
        }

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