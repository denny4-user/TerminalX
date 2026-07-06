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

static void screen_clone() {
    while (true) {
        ir_free(cloneSig);
        ir_rx_start();

        // Wait for a signal, showing a live activity meter: if pressing the
        // remote makes "RX" climb, the receiver is picking it up.
        uint32_t edges = 0;
        int last = ir_rx_level();
        uint32_t lastDraw = 0;
        bool captured = false;

        while (!captured) {
            board_update();
            if (nav_long) {
                ir_rx_stop();
                ir_free(cloneSig);
                return;
            }

            int v = ir_rx_level();
            if (v != last) {
                edges++;
                last = v;
            }

            if (ir_rx_poll(cloneSig)) {
                captured = true;
                break;
            }

            if (millis() - lastDraw > 120) {
                lastDraw = millis();
                char l[40];
                snprintf(l, sizeof(l), "RX: %lu", (unsigned long)edges);
                ui_message("Point remote & press", l, "waiting signal / hold NAV=back");
            }
        }
        ir_rx_stop();

        // Captured -> action screen
        bool rescan = false;
        while (!rescan) {
            char info[48];
            snprintf(info, sizeof(info), "%s  %u", cloneSig.proto.c_str(), cloneSig.len);
            ui_message("CAPTURED!", info, "front=SEND  side=new  hold=back");

            board_update();
            while (true) {
                board_update();
                if (ok_click) {
                    ir_replay(cloneSig);
                    ui_message("SENT ->", info, "front=send again  side=new");
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
