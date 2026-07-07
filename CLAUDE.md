# TerminalX — project & interface contract

TerminalX is custom firmware for the **M5Stack StickS3** (ESP32-S3, 8 MB): a pocket
"Swiss-army knife" multitool. Own minimal UI; feature *engines* are borrowed from the
proven **Bruce** firmware (fork Bruce-A-C-Edition) but always re-wrapped in TerminalX's
own UI. Philosophy: **maximum simplicity, best optimization, no bugs**, good on-screen
hints, and low battery drain. Features are added **one at a time, tested on real hardware**.

Final vision: many functions — **IR, RF (SubGHz), WiFi, BLE, …** — each with several
attack/action variants, each action conveniently configurable, all lean and battery-aware.

---

## 1. Controls — the only 4 actions (never change this model)

| Action | Button |
|---|---|
| **Select / Confirm / Execute** | **M5 main (front) button** — `ok_click` (GPIO11) |
| **Down / next item** | **single** press of the side button — `nav_click` (GPIO12) |
| **Up / previous item** | **double** press of the side button — `nav_double` |
| **Back one level** | **hold** the side button — `nav_long` |

- Input edges live in `board.cpp` (`ok_click`, `nav_click`, `nav_double`, `nav_long`).
  Reuse them; don't read pins directly in features.
- **Back = exactly one level.** Nested submenus must never over-pop or leak state.
  Enforce this by giving **every menu level its own blocking `while(true)` loop** that
  returns on `nav_long`/`-1`. A function that shows one menu and calls a child must loop,
  so that returning from the child lands back on that menu, not two levels up.

## 2. Running / action screens (attacks, captures, etc.)

- Draw a full frame once on entry (`M5.Display.fillScreen(COL_BG)`), then update in place
  with `ui_live(...)` (no full clears → no flicker).
- **Every running screen MUST show a bottom hint line in small font** telling the user how
  to stop/exit. Example for Beacon Spam: bottom line `M5=stop   hold side=exit`.
- Both **M5 (`ok_click`) and hold-side (`nav_long`) stop the action** and return to its
  submenu. Show the live counter (frames/packets) on the value line above the hint.

## 3. Visual design (keep the current look)

- Header: title (accent green) top-left, battery `NN%` top-right, divider `drawFastHLine`
  at y=20. Body below. Colors from `config.h` (`COL_BG/FG/ACCENT/DIM/WARN`), terminal-green
  accent on black.
- List menu: one item per row, selected row = filled rounded rect in accent, rest plain.
- **Font: `fonts::efontJA_16`** (efont base includes Latin **+ Cyrillic**), set via the
  `useFont()` helper in `ui.cpp`. This is why Russian SSIDs render instead of blank boxes.
  Use `textWidth()` for right-alignment, never `strlen*px`.
- Shared UI primitives live in `ui.cpp` (`ui_menu`, `ui_message`, `ui_progress`,
  `ui_live`, `ui_wait_any`). Build features on these; don't hand-roll menus.

## 4. Boot splash (`ui_splash`)

Short **"matrix" animation**: columns of digits rising **from the bottom upward**, digits
changing each frame (accent-green head, dimmer trail), ~1.2–1.5 s. Then clear and reveal
**`TerminalX`** centered with its **version** (`TERMINALX_VERSION`) below. Keep it brief and
cheap (no heavy buffers). Runs once in `setup()`.

## 5. Settings model

**Global settings** (device-wide, own top-level "Settings" entry; persist across reboot,
e.g. NVS/Preferences):
- Brightness
- Volume (future)
- Save location: **SD card** or **internal** memory
- FastBoot
- …(grow over time)

**Local settings** = per-function submenu named **`Config`** (e.g. `WiFi → Config`):
holds options specific to that function (e.g. WiFi: show/hide **hidden** networks).
Do **not** put channel selection in Config — channel is shown when you open a network.

> **META-RULE — ask first:** before adding any new *local* setting to a function's
> `Config` submenu, **ask the user** whether to add it. Don't add local options silently.

## 6. Engineering conventions

- Borrow proven engines from Bruce, but keep TerminalX's UI and structure. Never vendor
  all of Bruce — take only the needed function's code.
- Optimize for flash, RAM, and **battery**: turn radios off when leaving a feature
  (`WiFi.mode(WIFI_OFF)` etc.), avoid busy-spins, prefer in-place redraws.
- Build/flash specifics (pio path, **DIO flash mode is mandatory**, web-flasher merge) are
  in the project memory files — follow them.
