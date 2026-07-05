#include <Arduino.h>
#include "board.h"
#include "ui.h"
#include "ir.h"
#include "tvbgone.h"
#include "config.h"

// ---------- IR: TV-B-Gone ----------
static void screen_tvbgone() {
    const char *regions[] = {"All (NA + EU)", "Region NA", "Region EU"};
    int c = ui_menu("TV Power Off", regions, 3);
    if (c < 0) return;
    TvRegion reg = (c == 0) ? TV_BOTH : (c == 1) ? TV_NA : TV_EU;

    ui_progress("TV-B-Gone", 0, 1, "starting...");
    bool done = tvbgone_run(reg);
    ui_message("TV-B-Gone", done ? "Done." : "Stopped.", "Press any button");
    ui_wait_any();
}

// ---------- IR: Clone remote ----------
static IRSignal cloneSig;

static bool clone_cancel() {
    board_update();
    return nav_long;
}

static void screen_clone() {
    while (true) {
        ui_message("Clone Remote", "Aim remote, press", "a key   (hold NAV = back)");

        if (!ir_capture(cloneSig, 30000, clone_cancel)) {
            ir_free(cloneSig);
            return;   // timeout or cancelled
        }

        // Captured -> action screen
        bool rescan = false;
        while (!rescan) {
            char info[48];
            snprintf(info, sizeof(info), "%s  %u", cloneSig.proto.c_str(), cloneSig.len);
            ui_message("Captured", info, "OK = transmit   NAV = new");

            board_update();
            while (true) {
                board_update();
                if (ok_click) {
                    ir_replay(cloneSig);
                    ui_message("Captured", "Transmitted!", "OK = again   NAV = new");
                }
                if (nav_click) {
                    rescan = true;
                    break;
                }
                if (nav_long) {
                    ir_free(cloneSig);
                    return;
                }
                delay(8);
            }
        }
    }
}

// ---------- Menus ----------
static void screen_ir() {
    while (true) {
        const char *items[] = {"TV Power Off", "Clone Remote"};
        int c = ui_menu("IR Remote", items, 2);
        if (c < 0) return;
        if (c == 0) screen_tvbgone();
        else if (c == 1) screen_clone();
    }
}

static void screen_about() {
    ui_message("TerminalX", "v" TERMINALX_VERSION, "M5StickS3  -  IR toolkit");
    ui_wait_any();
}

void setup() {
    board_init();
    ir_init();

    // splash
    ui_message("TerminalX", "v" TERMINALX_VERSION, "loading...");
    delay(700);
}

void loop() {
    const char *items[] = {"IR Remote", "About", "Power Off"};
    int c = ui_menu("TerminalX", items, 3);
    if (c == 0) screen_ir();
    else if (c == 1) screen_about();
    else if (c == 2) {
        ui_message("TerminalX", "Powering off...", nullptr);
        delay(600);
        board_power_off();
    }
}
