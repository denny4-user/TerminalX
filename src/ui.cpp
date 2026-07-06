#include "ui.h"
#include "board.h"
#include "config.h"

#define SCR_W 240
#define SCR_H 135

static void draw_header(const char *title) {
    auto &D = M5.Display;
    D.fillScreen(COL_BG);
    D.setTextSize(2);
    D.setTextColor(COL_ACCENT, COL_BG);
    D.setCursor(4, 3);
    D.print(title);
    // battery, top-right
    D.setTextSize(1);
    D.setTextColor(COL_DIM, COL_BG);
    char b[8];
    snprintf(b, sizeof(b), "%d%%", board_battery());
    int w = strlen(b) * 6;
    D.setCursor(SCR_W - w - 3, 6);
    D.print(b);
    D.drawFastHLine(0, 20, SCR_W, COL_DIM);
}

static void draw_menu(const char *title, const char *const *items, int count, int sel) {
    auto &D = M5.Display;
    draw_header(title);

    const int top = 26;
    const int rowH = 22;
    const int maxRows = (SCR_H - top) / rowH;   // ~4-5 rows

    int first = 0;
    if (sel >= maxRows) first = sel - maxRows + 1;

    D.setTextSize(2);
    for (int i = 0; i < maxRows && (first + i) < count; i++) {
        int idx = first + i;
        int y = top + i * rowH;
        if (idx == sel) {
            D.fillRoundRect(2, y - 1, SCR_W - 4, rowH - 2, 3, COL_ACCENT);
            D.setTextColor(COL_BG, COL_ACCENT);
        } else {
            D.setTextColor(COL_FG, COL_BG);
        }
        D.setCursor(8, y + 2);
        D.print(items[idx]);
    }
}

int ui_menu(const char *title, const char *const *items, int count, int start) {
    int sel = start;
    bool dirty = true;
    board_update();   // clear stale edges
    while (true) {
        board_update();
        if (nav_click) {                       // single side tap -> down
            sel = (sel + 1) % count;
            dirty = true;
        }
        if (nav_double) {                      // double side tap -> up
            sel = (sel - 1 + count) % count;
            dirty = true;
        }
        if (ok_click) return sel;              // front button -> select
        if (nav_long) return -1;               // hold side -> back
        if (dirty) {
            draw_menu(title, items, count, sel);
            dirty = false;
        }
        delay(8);
    }
}

void ui_message(const char *title, const char *line1, const char *line2) {
    auto &D = M5.Display;
    draw_header(title);
    D.setTextSize(2);
    D.setTextColor(COL_FG, COL_BG);
    if (line1) {
        D.setCursor(8, 54);
        D.print(line1);
    }
    if (line2) {
        D.setTextSize(1);
        D.setTextColor(COL_DIM, COL_BG);
        D.setCursor(8, 84);
        D.print(line2);
    }
}

void ui_progress(const char *title, int cur, int total, const char *hint) {
    auto &D = M5.Display;
    draw_header(title);

    if (total < 1) total = 1;
    if (cur > total) cur = total;
    int pct = (cur * 100) / total;

    const int bx = 10, by = 60, bw = SCR_W - 20, bh = 20;
    D.drawRoundRect(bx, by, bw, bh, 4, COL_FG);
    int fw = ((bw - 4) * pct) / 100;
    if (fw > 0) D.fillRoundRect(bx + 2, by + 2, fw, bh - 4, 3, COL_ACCENT);

    D.setTextSize(2);
    D.setTextColor(COL_FG, COL_BG);
    char l[32];
    snprintf(l, sizeof(l), "%d / %d  (%d%%)", cur, total, pct);
    D.setCursor(10, 90);
    D.print(l);

    if (hint) {
        D.setTextSize(1);
        D.setTextColor(COL_DIM, COL_BG);
        D.setCursor(10, 118);
        D.print(hint);
    }
}

void ui_live(const char *title, const char *line1, const char *line2) {
    auto &D = M5.Display;
    // Header, drawn opaque so it overwrites the previous frame (no clear/flash).
    D.setTextSize(2);
    D.setTextColor(COL_ACCENT, COL_BG);
    D.setCursor(4, 3);
    D.printf("%-14s", title);
    D.setTextSize(1);
    D.setTextColor(COL_DIM, COL_BG);
    char b[8];
    snprintf(b, sizeof(b), "%3d%%", board_battery());
    D.setCursor(SCR_W - 4 * 6 - 3, 6);
    D.print(b);
    D.drawFastHLine(0, 20, SCR_W, COL_DIM);

    // Body lines, space-padded so leftovers from longer text get erased.
    D.setTextSize(2);
    D.setTextColor(COL_FG, COL_BG);
    D.setCursor(8, 54);
    D.printf("%-19s", line1 ? line1 : "");
    D.setTextSize(1);
    D.setTextColor(COL_DIM, COL_BG);
    D.setCursor(8, 84);
    D.printf("%-39s", line2 ? line2 : "");
}

void ui_wait_any() {
    board_update();
    while (true) {
        board_update();
        if (ok_click || nav_click || nav_long) return;
        delay(8);
    }
}
