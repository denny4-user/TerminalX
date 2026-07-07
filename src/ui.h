#pragma once
#include <Arduino.h>

// Boot splash: brief "matrix" of digits rising bottom->top, then reveals
// "TerminalX" and the given version. Blocks for ~2s. Runs once at startup.
void ui_splash(const char *version);

// Blocking list menu. Returns selected index, or -1 if Back (long NAV).
// If `bars` is non-null (one 0..4 level per item), a signal-strength icon is
// drawn to the left of each row (used by the WiFi scan list).
int ui_menu(const char *title, const char *const *items, int count, int start = 0,
            const uint8_t *bars = nullptr);

// Static screens (draw once, caller handles input).
void ui_message(const char *title, const char *line1, const char *line2 = nullptr);
void ui_progress(const char *title, int cur, int total, const char *hint);

// Live status screen: overwrites text in place (no full-screen clear -> no
// flicker). Safe to call every frame from a running attack loop.
void ui_live(const char *title, const char *line1, const char *line2 = nullptr);

// Static bottom hint line (small font). Draw once after fillScreen when entering
// an action/running screen; ui_live leaves this region untouched. Per the UI
// contract, every running screen shows how to stop/exit here.
void ui_hint(const char *text);

// Wait for any button; returns which. Convenience for message screens.
void ui_wait_any();
