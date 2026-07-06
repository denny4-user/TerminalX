#include "ir.h"
#include "config.h"

#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRutils.h>

static IRsend irsend(IR_TX_PIN);

// Receiver params matched to Bruce's working setup on this board:
// IRrecv(pin, buffer, timeout=50ms).
static IRrecv irrecv(IR_RX_PIN, 1024, 50);
static decode_results results;

void ir_init() {
    irsend.begin();
    pinMode(IR_TX_PIN, OUTPUT);
    digitalWrite(IR_TX_PIN, LOW);
}

void ir_rx_start() {
    irrecv.enableIRIn();
    pinMode(IR_RX_PIN, INPUT_PULLUP);   // Bruce: setup_ir_pin(irRx, INPUT_PULLUP)
}

void ir_rx_stop() { irrecv.disableIRIn(); }

int ir_rx_level() { return digitalRead(IR_RX_PIN); }

bool ir_rx_poll(IRSignal &sig) {
    if (!irrecv.decode(&results)) return false;

    uint16_t len = getCorrectedRawLength(&results);
    uint16_t *raw = resultToRawArray(&results);
    bool ok = false;
    if (raw && len) {
        ir_free(sig);
        sig.raw = raw;
        sig.len = len;
        sig.freq = 38;
        sig.proto = typeToString(results.decode_type, results.repeat);
        ok = true;
    } else if (raw) {
        delete[] raw;
    }
    irrecv.resume();
    return ok;
}

void ir_replay(const IRSignal &sig) {
    if (!sig.valid()) return;
    irsend.sendRaw(sig.raw, sig.len, sig.freq);
}

void ir_free(IRSignal &sig) {
    if (sig.raw) {
        delete[] sig.raw;
        sig.raw = nullptr;
    }
    sig.len = 0;
}

void ir_send_raw(const uint16_t *buf, uint16_t len, uint16_t khz) {
    // sendRaw's buffer param is const in modern forks, non-const in older ones;
    // const_cast keeps this compiling against either signature.
    irsend.sendRaw(const_cast<uint16_t *>(buf), len, khz);
}
