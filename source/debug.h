#pragma once

#include <stdbool.h>

void initDebugMenu(void);
void updateDebugMenu(void);
void drawDebugMenu(void);

// True while the menu is on screen. main.c uses this to ignore gameplay
// input (stick, jump, etc.) so the pad only drives the menu.
bool debugMenuIsOpen(void);

// Returns true ONCE after the player closed the menu with Y (apply).
// main.c uses it to throw away the old islands and generate new ones
// using the freshly applied values.
bool debugMenuConsumeApply(void);

bool debugMenuConsumeReset(void);