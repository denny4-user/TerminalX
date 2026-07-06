#include "board.h"
#include "config.h"

bool ok_click = false;
bool nav_click = false;
bool nav_double = false;
bool nav_long = false;

static const uint32_t DEBOUNCE_MS = 20;
static const uint32_t LONG_MS = 500;    // hold side button this long = Back
static const uint32_t OK_MAX_MS = 900;  // longer than this = ignore (not a tap)
static const uint32_t DOUBLE_MS = 250;  // 2nd side tap within this = double-click

static bool okDown = false;
static uint32_t okAt = 0;

static bool navDown = false;
static uint32_t navAt = 0;
static bool navLongFired = false;

// Deferred single/double click: count taps, then commit once the gap passes.
static uint8_t navClicks = 0;
static uint32_t navLastRelease = 0;

void board_init() {
    Serial.begin(115200);
    Serial.printf("\n[TerminalX] v%s boot\n", TERMINALX_VERSION);

    auto cfg = M5.config();
    M5.begin(cfg);                     // autodetects M5StickS3, powers the LCD

    // Backlight PWM on GPIO38 (StickS3 drives its backlight here manually)
    pinMode(TFT_BL_PIN, OUTPUT);
    ledcAttach(TFT_BL_PIN, 5000, 8);
    ledcWrite(TFT_BL_PIN, 255);
    M5.Display.setBrightness(255);
    M5.Display.setRotation(3);         // landscape 240x135
    M5.Display.fillScreen(COL_BG);

    Serial.printf(
        "[TerminalX] display %dx%d board=%d\n",
        (int)M5.Display.width(), (int)M5.Display.height(), (int)M5.getBoard()
    );

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
}

void board_update() {
    M5.update();
    ok_click = nav_click = nav_double = nav_long = false;
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

    // ---- NAV (side) : single tap / double tap / long hold ----
    if (navRaw && !navDown) {
        navDown = true;
        navAt = now;
        navLongFired = false;
    } else if (navRaw && navDown) {
        // Hold = Back. Fires immediately, cancels any pending taps.
        if (!navLongFired && (now - navAt) >= LONG_MS) {
            nav_long = true;
            navLongFired = true;
            navClicks = 0;
        }
    } else if (!navRaw && navDown) {
        navDown = false;
        if (!navLongFired && (now - navAt) >= DEBOUNCE_MS) {
            navClicks++;
            navLastRelease = now;
        }
    }

    // Commit taps once the double-click window closes and the button is up.
    if (navClicks > 0 && !navDown && (now - navLastRelease) >= DOUBLE_MS) {
        if (navClicks >= 2) nav_double = true;  // Up / Prev
        else nav_click = true;                  // Down / Next
        navClicks = 0;
    }
}

int board_battery() {
    int l = M5.Power.getBatteryLevel();
    return (l < 0) ? 0 : (l > 100 ? 100 : l);
}

void board_power_off() { M5.Power.powerOff(); }
