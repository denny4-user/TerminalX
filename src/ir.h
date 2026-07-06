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

// Non-blocking receive.
void ir_rx_start();               // enable receiver (INPUT_PULLUP on RX)
void ir_rx_stop();                // disable receiver
int  ir_rx_level();               // raw RX pin level (live-activity meter)
bool ir_rx_poll(IRSignal &sig);   // true once a full signal is captured

void ir_replay(const IRSignal &sig);
void ir_free(IRSignal &sig);

// Shared low-level sender (used by TV-B-Gone). khz = carrier frequency.
void ir_send_raw(const uint16_t *buf, uint16_t len, uint16_t khz);
