#pragma once
#include <M5Unified.h>

// Hardware bring-up and input for M5StickS3.
void board_init();
void board_update();     // call every frame: refreshes M5 + button edges below

int  board_battery();    // 0..100
void board_power_off();

// Button edge events. Valid only for the frame after board_update().
extern bool ok_click;    // front button, short press
extern bool nav_click;   // side button, short press  -> Next
extern bool nav_long;    // side button, long press   -> Back
