#include "board.h"
#include "config.h"

bool ok_click = false;
bool nav_click = false;
bool nav_long = false;

static const uint32_t DEBOUNCE_MS = 20;
static const uint32_t LONG_MS = 500;
static const uint32_t OK_MAX_MS = 900;  // longer than this = ignore (not a tap)

static bool okDown = false;
static uint32_t okAt = 0;

static bool navDown = false;
static uint32_t navAt = 0;
static bool navLongFired = false;

void board_init() {
    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Display.setRotation(1);         // landscape 240x135
    M5.Display.fillScreen(COL_BG);

    M5.Power.setExtOutput(false);      // stop the buzzing on ext power

    // Keep unused SPI peripherals quiet (SD / CC1101 / nRF / PN532 CS high)
    const int csPins[] = {7, 2, 8, 43};
    for (int p : csPins) {
        pinMode(p, OUTPUT);
        digitalWrite(p, HIGH);
    }
    pinMode(9, OUTPUT);
    digitalWrite(9, LOW);              // avoid M5RF433 jamming

    pinMode(BTN_OK_PIN, INPUT_PULLUP);
    pinMode(BTN_NAV_PIN, INPUT_PULLUP);

    // Backlight PWM on GPIO38
    ledcAttach(TFT_BL_PIN, 5000, 8);
    ledcWrite(TFT_BL_PIN, 250);
}

void board_update() {
    M5.update();
    ok_click = nav_click = nav_long = false;
    uint32_t now = millis();

    bool okRaw = (digitalRead(BTN_OK_PIN) == LOW);
    bool navRaw = (digitalRead(BTN_NAV_PIN) == LOW);

    // ---- OK (front) : short tap on release ----
    if (okRaw && !okDown) {
        okDown = true;
        okAt = now;
    } else if (!okRaw && okDown) {
        okDown = false;
        uint32_t held = now - okAt;
        if (held >= DEBOUNCE_MS && held < OK_MAX_MS) ok_click = true;
    }

    // ---- NAV (side) : short tap OR long press ----
    if (navRaw && !navDown) {
        navDown = true;
        navAt = now;
        navLongFired = false;
    } else if (navRaw && navDown) {
        if (!navLongFired && (now - navAt) >= LONG_MS) {
            nav_long = true;
            navLongFired = true;
        }
    } else if (!navRaw && navDown) {
        navDown = false;
        if (!navLongFired && (now - navAt) >= DEBOUNCE_MS) nav_click = true;
    }
}

int board_battery() {
    int l = M5.Power.getBatteryLevel();
    return (l < 0) ? 0 : (l > 100 ? 100 : l);
}

void board_power_off() { M5.Power.powerOff(); }
