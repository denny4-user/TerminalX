#pragma once
#include <Arduino.h>

// A captured IR signal, stored as raw mark/space timings (microseconds).
struct IRSignal {
    uint16_t *raw = nullptr;
    uint16_t len = 0;
    uint16_t freq = 38;      // kHz carrier used for replay
    String proto = "RAW";
    bool valid() const { return raw != nullptr && len > 0; }
};

void ir_init();

// Capture one button. Blocks up to timeout_ms. `cancel` (may be null) is polled
// repeatedly; if it returns true, capture aborts. Returns true on success.
bool ir_capture(IRSignal &sig, uint32_t timeout_ms, bool (*cancel)());

void ir_replay(const IRSignal &sig);
void ir_free(IRSignal &sig);

// Shared low-level sender (used by TV-B-Gone). khz = carrier frequency.
void ir_send_raw(const uint16_t *buf, uint16_t len, uint16_t khz);
