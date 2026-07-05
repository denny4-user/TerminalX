#include "ir.h"
#include "config.h"

#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRutils.h>

static IRsend irsend(IR_TX_PIN);

static const uint16_t kIrCaptureBufSize = 1024;
static const uint8_t kIrCaptureTimeout = 15;  // ms
static IRrecv irrecv(IR_RX_PIN, kIrCaptureBufSize, kIrCaptureTimeout, true);
static decode_results results;

void ir_init() {
    irsend.begin();
    pinMode(IR_TX_PIN, OUTPUT);
    digitalWrite(IR_TX_PIN, LOW);
}

bool ir_capture(IRSignal &sig, uint32_t timeout_ms, bool (*cancel)()) {
    ir_free(sig);
    irrecv.enableIRIn();

    bool got = false;
    uint32_t start = millis();
    while (millis() - start < timeout_ms) {
        if (cancel && cancel()) break;
        if (irrecv.decode(&results)) {
            uint16_t len = getCorrectedRawLength(&results);
            uint16_t *raw = resultToRawArray(&results);
            if (raw && len) {
                sig.raw = raw;
                sig.len = len;
                sig.freq = 38;
                sig.proto = typeToString(results.decode_type, results.repeat);
                got = true;
            } else if (raw) {
                delete[] raw;
            }
            irrecv.resume();
            break;
        }
        delay(5);
    }

    irrecv.disableIRIn();
    return got;
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
