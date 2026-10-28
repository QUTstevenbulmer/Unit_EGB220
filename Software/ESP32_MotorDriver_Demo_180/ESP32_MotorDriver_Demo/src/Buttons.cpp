// =============================================================================
//  Buttons.cpp  -  SW1, SW2 (breakout) and BOOT (controller), debounced.
//  All three read LOW when pressed.
// =============================================================================
#include "Breakout.h"
#include "pins.h"

struct Btn {
    uint8_t  pin;
    bool     stable = false;      // debounced state (true = down)
    bool     lastRaw = false;
    uint32_t changedAt = 0;       // when raw last changed
    uint32_t downAt = 0;          // when the press started
    bool     longFired = false;
    bool     shortEvent = false;
    bool     longEvent = false;
};

static Btn btns[BTN_COUNT];
static const uint32_t DEBOUNCE_MS = 25;
static const uint32_t LONG_MS     = 1000;

void buttonsBegin() {
    btns[BTN_SW1].pin  = PIN_SW1;
    btns[BTN_SW2].pin  = PIN_SW2;
    btns[BTN_BOOT].pin = PIN_BOOT_BUTTON;
    for (auto& b : btns) pinMode(b.pin, INPUT_PULLUP);
}

void buttonsUpdate() {
    uint32_t now = millis();
    for (auto& b : btns) {
        bool raw = digitalRead(b.pin) == LOW;
        if (raw != b.lastRaw) { b.lastRaw = raw; b.changedAt = now; }

        if (now - b.changedAt >= DEBOUNCE_MS && raw != b.stable) {
            b.stable = raw;
            if (raw) { b.downAt = now; b.longFired = false; }       // pressed
            else if (!b.longFired) b.shortEvent = true;             // released
        }
        if (b.stable && !b.longFired && now - b.downAt >= LONG_MS) {
            b.longFired = true;
            b.longEvent = true;
        }
    }
}

bool buttonDown(ButtonId id) { return btns[id].stable; }

bool buttonPressed(ButtonId id) {
    bool e = btns[id].shortEvent; btns[id].shortEvent = false; return e;
}

bool buttonLongPressed(ButtonId id) {
    bool e = btns[id].longEvent; btns[id].longEvent = false; return e;
}
