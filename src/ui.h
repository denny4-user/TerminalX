#pragma once
#include <Arduino.h>

// Blocking list menu. Returns selected index, or -1 if Back (long NAV).
int ui_menu(const char *title, const char *const *items, int count, int start = 0);

// Static screens (draw once, caller handles input).
void ui_message(const char *title, const char *line1, const char *line2 = nullptr);
void ui_progress(const char *title, int cur, int total, const char *hint);

// Wait for any button; returns which. Convenience for message screens.
void ui_wait_any();
