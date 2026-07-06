#pragma once
#include <Arduino.h>

// Blocking list menu. Returns selected index, or -1 if Back (long NAV).
int ui_menu(const char *title, const char *const *items, int count, int start = 0);

// Static screens (draw once, caller handles input).
void ui_message(const char *title, const char *line1, const char *line2 = nullptr);
void ui_progress(const char *title, int cur, int total, const char *hint);

// Live status screen: overwrites text in place (no full-screen clear -> no
// flicker). Safe to call every frame from a running attack loop.
void ui_live(const char *title, const char *line1, const char *line2 = nullptr);

// Wait for any button; returns which. Convenience for message screens.
void ui_wait_any();
