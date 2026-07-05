#include "tvbgone.h"
#include "ir.h"
#include "ui.h"
#include "board.h"
#include "config.h"
#include <Arduino.h>

// The compressed TV power-code database (self-contained).
// NOTE: included in this translation unit ONLY (it defines global arrays).
#include "WORLD_IR_CODES.h"

static const uint16_t numNA = sizeof(NApowerCodes) / sizeof(NApowerCodes[0]);
static const uint16_t numEU = sizeof(EUpowerCodes) / sizeof(EUpowerCodes[0]);

// ---- bit-unpacker (from the original TV-B-Gone) ----
static const IrCode *powerCode;
static uint8_t bitsleft_r = 0;
static uint8_t bits_r = 0;
static uint8_t code_ptr = 0;

static uint8_t read_bits(uint8_t count) {
    uint8_t tmp = 0;
    while (count--) {
        if (bitsleft_r == 0) {
            bits_r = powerCode->codes[code_ptr++];
            bitsleft_r = 8;
        }
        tmp = (tmp << 1) | ((bits_r >> --bitsleft_r) & 1);
    }
    return tmp;
}

static uint16_t rawData[520];   // numpairs (<=255) * 2

static void send_code(const IrCode *pc) {
    powerCode = pc;
    const uint16_t freq = pc->timer_val;          // already kHz
    const uint8_t numpairs = pc->numpairs;
    const uint8_t bitcompression = pc->bitcompression;

    code_ptr = 0;
    bitsleft_r = 0;
    bits_r = 0;
    for (uint8_t k = 0; k < numpairs; k++) {
        uint16_t ti = read_bits(bitcompression) * 2;
        rawData[k * 2] = pc->times[ti] * 10;          // mark  (us)
        rawData[k * 2 + 1] = pc->times[ti + 1] * 10;  // space (us)
    }
    ir_send_raw(rawData, (uint16_t)(numpairs * 2), freq);
}

// Non-blocking gap that keeps input responsive. Returns true if cancelled.
static bool gap_or_cancel(uint32_t ms) {
    uint32_t start = millis();
    while (millis() - start < ms) {
        board_update();
        if (ok_click || nav_long) return true;
        delay(5);
    }
    return false;
}

bool tvbgone_run(TvRegion region) {
    struct Phase {
        const IrCode *const *arr;
        uint16_t n;
    } phases[2];
    int np = 0;
    if (region == TV_NA || region == TV_BOTH) phases[np++] = {NApowerCodes, numNA};
    if (region == TV_EU || region == TV_BOTH) phases[np++] = {EUpowerCodes, numEU};

    uint16_t total = 0;
    for (int p = 0; p < np; p++) total += phases[p].n;

    uint16_t sent = 0;
    bool cancelled = false;

    for (int p = 0; p < np && !cancelled; p++) {
        for (uint16_t i = 0; i < phases[p].n; i++) {
            send_code(phases[p].arr[i]);
            sent++;
            if (sent % 3 == 0 || sent == total)
                ui_progress("TV-B-Gone", sent, total, "OK / hold NAV = stop");

            if (gap_or_cancel(205)) {   // ~205 ms between codes
                cancelled = true;
                break;
            }
        }
    }

    digitalWrite(IR_TX_PIN, LOW);       // ensure LED off
    return !cancelled;
}
