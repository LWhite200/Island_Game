#include <gccore.h>
#include <math.h>
#include "common.h"

void drawWater(float time) {
    // Original water drawing code exactly as you wrote it
    GX_Begin(GX_QUADS, GX_VTXFMT0, (WATER_SIZE - 1) * (WATER_SIZE - 1) * 4);

    for (int i = 0; i < WATER_SIZE - 1; i++) {
        for (int j = 0; j < WATER_SIZE - 1; j++) {
            f32 x0 = i - WATER_SIZE / 2;
            f32 z0 = j - WATER_SIZE / 2;
            f32 x1 = i + 1 - WATER_SIZE / 2;
            f32 z1 = j - WATER_SIZE / 2;
            f32 x2 = i + 1 - WATER_SIZE / 2;
            f32 z2 = j + 1 - WATER_SIZE / 2;
            f32 x3 = i - WATER_SIZE / 2;
            f32 z3 = j + 1 - WATER_SIZE / 2;

            f32 y0 = sinf((x0 + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE + cosf((z0 + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE;
            f32 y1 = sinf((x1 + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE + cosf((z1 + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE;
            f32 y2 = sinf((x2 + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE + cosf((z2 + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE;
            f32 y3 = sinf((x3 + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE + cosf((z3 + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE;

            float waveHeight = (y0 + y1 + y2 + y3) / 4.0f;
            float r = 0.0f, g = 0.0f, b = 0.0f;

            if (waveHeight > -1.0f) {
                r = 0.05f + 0.2f * waveHeight;
                g = 0.1f + 0.2f * waveHeight;
                b = 0.8f + 0.2f * waveHeight;
            }
            else {
                r = 0.0f;
                g = 0.0f;
                b = 0.7f;
            }

            float timeShift = sinf(time * 0.1f);
            r += 0.1f * timeShift;
            g += 0.05f * timeShift;
            b += 0.1f * timeShift;

            r = fminf(1.0f, fmaxf(0.0f, r));
            g = fminf(1.0f, fmaxf(0.0f, g));
            b = fminf(1.0f, fmaxf(0.0f, b));

            GX_Position3f32(x0, y0, z0);
            GX_Color3f32(r, g, b);
            GX_Position3f32(x1, y1, z1);
            GX_Color3f32(r, g, b);
            GX_Position3f32(x2, y2, z2);
            GX_Color3f32(r, g, b);
            GX_Position3f32(x3, y3, z3);
            GX_Color3f32(r, g, b);
        }
    }

    GX_End();
}