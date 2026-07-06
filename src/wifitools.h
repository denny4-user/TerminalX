#pragma once

// TerminalX WiFi toolkit.
//
// Injection engine (raw 802.11 frame TX, beacon/deauth templates) is borrowed
// from the Bruce firmware (Bruce-A-C-Edition), wrapped in TerminalX's own UI.
// Requires the net80211 raw-frame sanity check to be bypassed at link time
// (see -Wl,--allow-multiple-definition in platformio.ini + the override in the
// .cpp). For M5Stack StickS3 only.

// Entry point from the main menu. Runs its own submenu loop; returns on Back.
void wifitools_menu();
