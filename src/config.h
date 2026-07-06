#pragma once
#include <stdint.h>

// ================= TerminalX =================
// Firmware for M5Stack StickS3 (ESP32-S3). IR-first multitool.
// Philosophy: minimal, optimized, bug-free.
// =============================================

#define TERMINALX_VERSION "0.3.0"

// ---- Pins (M5StickS3) ----------------------------------------
#ifndef IR_TX_PIN
#define IR_TX_PIN 46      // internal IR LED (active HIGH)
#endif
#ifndef IR_RX_PIN
#define IR_RX_PIN 42      // internal IR receiver
#endif
#ifndef BTN_OK_PIN
#define BTN_OK_PIN 11     // front "M5" button  -> OK / Select
#endif
#ifndef BTN_NAV_PIN
#define BTN_NAV_PIN 12    // side button        -> Next (short) / Back (long)
#endif
#ifndef TFT_BL_PIN
#define TFT_BL_PIN 38     // display backlight (PWM)
#endif

// ---- Theme (RGB565) ------------------------------------------
#define COL_BG      0x0000   // black
#define COL_FG      0xFFFF   // white
#define COL_ACCENT  0x07E0   // green (terminal)
#define COL_DIM     0x8410   // gray
#define COL_WARN    0xFD20   // orange
