#include "ui.h"
#include "board.h"
#include "config.h"

#define SCR_W 240
#define SCR_H 135

// Two fonts:
//  - useFont():  efont 16px. Its base set includes Latin + Cyrillic, so Russian
//    SSIDs render instead of blank boxes. Used for titles, menus, body text.
//  - useSmall(): built-in 6x8 GLCD font for small ASCII-only text (battery,
//    hints, counters). Genuinely small and costs no extra flash.
static inline void useFont()  { M5.Display.setFont(&fonts::efontJA_16); M5.Display.setTextSize(1); }
static inline void useMenu()  { M5.Display.setFont(&fonts::efontJA_10); M5.Display.setTextSize(1); }
static inline void useSmall() { M5.Display.setFont(&fonts::Font0);      M5.Display.setTextSize(1); }

// Small 4-bar signal-strength icon; `level` 0..4 bars filled.
static void draw_signal(int x, int yBottom, int level, uint16_t filled, uint16_t empty) {
    for (int b = 0; b < 4; b++) {
        int bh = 3 + b * 2;                          // 3,5,7,9 px
        M5.Display.fillRect(x + b * 3, yBottom - bh, 2, bh, (b < level) ? filled : empty);
    }
}

void ui_splash(const char *version) {
    auto &D = M5.Display;
    D.fillScreen(COL_BG);

    const int CW = 10;                    // column width (px)
    const int CH = 12;                    // cell height (px)
    const int cols = SCR_W / CW;          // ~24 columns
    const int rows = SCR_H / CH + 1;      // visible rows
    int head[SCR_W / CW + 1];
    for (int c = 0; c < cols; c++) head[c] = -(int)random(rows);  // start above screen

    // Phase 1: digits rain DOWNWARD.
    useSmall();
    uint32_t t0 = millis();
    while (millis() - t0 < 1000) {
        for (int c = 0; c < cols; c++) {
            int x = c * CW, h = head[c];
            int cy = (h - 3) * CH;                       // cell vacated above the trail
            if (cy >= 0 && cy < SCR_H) D.fillRect(x, cy, CW, CH, COL_BG);
            for (int t = 0; t < 3; t++) {                // head (bright, leading) + trail above
                int y = (h - t) * CH;
                if (y < 0 || y >= SCR_H) continue;
                D.setTextColor((t == 0) ? COL_FG : (t == 1) ? COL_ACCENT : COL_DIM, COL_BG);
                D.setCursor(x + 2, y + 2);
                D.print((char)('0' + (int)random(10)));
            }
            if (++head[c] > rows + 3) head[c] = -(int)random(rows);   // respawn above
        }
        delay(45);
    }

    // Phase 2: the logo "decodes" out of the digits, left-to-right — each slot
    // flickers random digits, then locks into its real character.
    D.fillScreen(COL_BG);
    useFont();
    D.setTextSize(2);
    const char *name = "TerminalX";
    const int nlen = 9;
    const int charW = D.textWidth("0");
    const int nx = (SCR_W - nlen * charW) / 2;
    const int ny = 44;
    for (int resolved = 0; resolved <= nlen; resolved++) {
        D.fillRect(0, ny - 2, SCR_W, 36, COL_BG);
        for (int k = 0; k < nlen; k++) {
            char ch = (k < resolved) ? name[k] : (char)('0' + (int)random(10));
            D.setTextColor((k < resolved) ? COL_ACCENT : COL_FG, COL_BG);
            D.setCursor(nx + k * charW, ny);
            D.print(ch);
        }
        delay(85);
    }

    // Version below, then hold.
    D.setTextSize(1);
    useSmall();
    D.setTextColor(COL_DIM, COL_BG);
    char v[24];
    snprintf(v, sizeof(v), "v%s", version);
    D.setCursor((SCR_W - D.textWidth(v)) / 2, ny + 40);
    D.print(v);
    delay(750);
}

static void draw_header(const char *title) {
    auto &D = M5.Display;
    D.fillScreen(COL_BG);
    useFont();
    D.setTextColor(COL_ACCENT, COL_BG);
    D.setCursor(4, 2);
    D.print(title);
    // battery, top-right
    useSmall();
    D.setTextColor(COL_DIM, COL_BG);
    char b[8];
    snprintf(b, sizeof(b), "%d%%", board_battery());
    D.setCursor(SCR_W - D.textWidth(b) - 3, 7);
    D.print(b);
    D.drawFastHLine(0, 20, SCR_W, COL_DIM);
}

static void draw_menu(const char *title, const char *const *items, int count, int sel,
                      const uint8_t *bars) {
    auto &D = M5.Display;
    draw_header(title);

    const int top = 23;
    const int rowH = 13;
    const int maxRows = (SCR_H - top) / rowH;   // ~8 rows (small font)

    int first = 0;
    if (sel >= maxRows) first = sel - maxRows + 1;

    useMenu();
    for (int i = 0; i < maxRows && (first + i) < count; i++) {
        int idx = first + i;
        int y = top + i * rowH;
        bool s = (idx == sel);
        if (s) {
            D.fillRoundRect(2, y, SCR_W - 4, rowH - 1, 2, COL_ACCENT);
            D.setTextColor(COL_BG, COL_ACCENT);
        } else {
            D.setTextColor(COL_FG, COL_BG);
        }
        int tx = 8;
        if (bars) {                             // signal icon left of the SSID
            draw_signal(6, y + rowH - 3, bars[idx],
                        s ? COL_BG : COL_ACCENT, s ? COL_ACCENT : COL_DIM);
            tx = 24;
        }
        D.setCursor(tx, y + 2);
        D.print(items[idx]);
    }
}

int ui_menu(const char *title, const char *const *items, int count, int start,
            const uint8_t *bars) {
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
            draw_menu(title, items, count, sel, bars);
            dirty = false;
        }
        delay(8);
    }
}

void ui_message(const char *title, const char *line1, const char *line2) {
    auto &D = M5.Display;
    draw_header(title);
    if (line1) {
        useFont();
        D.setTextColor(COL_FG, COL_BG);
        D.setCursor(8, 52);
        D.print(line1);
    }
    if (line2) {
        useSmall();
        D.setTextColor(COL_DIM, COL_BG);
        D.setCursor(8, 90);
        D.print(line2);
    }
}

void ui_progress(const char *title, int cur, int total, const char *hint) {
    auto &D = M5.Display;
    draw_header(title);

    if (total < 1) total = 1;
    if (cur > total) cur = total;
    int pct = (cur * 100) / total;

    const int bx = 10, by = 56, bw = SCR_W - 20, bh = 20;
    D.drawRoundRect(bx, by, bw, bh, 4, COL_FG);
    int fw = ((bw - 4) * pct) / 100;
    if (fw > 0) D.fillRoundRect(bx + 2, by + 2, fw, bh - 4, 3, COL_ACCENT);

    useFont();
    D.setTextColor(COL_FG, COL_BG);
    char l[32];
    snprintf(l, sizeof(l), "%d / %d  (%d%%)", cur, total, pct);
    D.setCursor(10, 84);
    D.print(l);

    if (hint) {
        useSmall();
        D.setTextColor(COL_DIM, COL_BG);
        D.setCursor(10, 116);
        D.print(hint);
    }
}

void ui_live(const char *title, const char *line1, const char *line2) {
    auto &D = M5.Display;
    // Header: opaque redraw (constant text, no flicker, no full clear).
    useFont();
    D.setTextColor(COL_ACCENT, COL_BG);
    D.setCursor(4, 2);
    D.print(title);
    useSmall();
    D.setTextColor(COL_DIM, COL_BG);
    char b[8];
    snprintf(b, sizeof(b), "%d%%", board_battery());
    D.fillRect(SCR_W - 44, 0, 44, 18, COL_BG);
    D.setCursor(SCR_W - D.textWidth(b) - 3, 7);
    D.print(b);
    D.drawFastHLine(0, 20, SCR_W, COL_DIM);

    // Body lines: clear each region first so shrinking text leaves no remnants.
    useFont();
    D.setTextColor(COL_FG, COL_BG);
    D.fillRect(0, 50, SCR_W, 20, COL_BG);
    D.setCursor(8, 52);
    D.print(line1 ? line1 : "");
    useSmall();
    D.setTextColor(COL_DIM, COL_BG);
    D.fillRect(0, 84, SCR_W, 14, COL_BG);
    D.setCursor(8, 88);
    D.print(line2 ? line2 : "");
}

void ui_hint(const char *text) {
    auto &D = M5.Display;
    useSmall();
    D.setTextColor(COL_DIM, COL_BG);
    D.fillRect(0, 116, SCR_W, SCR_H - 116, COL_BG);
    D.setCursor(6, 120);
    D.print(text);
}

void ui_wait_any() {
    board_update();
    while (true) {
        board_update();
        if (ok_click || nav_click || nav_long) return;
        delay(8);
    }
}
