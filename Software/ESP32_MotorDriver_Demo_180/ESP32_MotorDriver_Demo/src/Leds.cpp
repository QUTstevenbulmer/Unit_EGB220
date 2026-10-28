// =============================================================================
//  Leds.cpp  -  The four LEDs on the breakout board.
//  Each LED is on its own PWM channel so brightness can be set 0..255.
// =============================================================================
#include "Breakout.h"
#include "App.h"
#include "pins.h"
#include "PwmCompat.h"

const char* const LED_NAMES[LED_COUNT] = { "red", "green", "blue", "yellow" };

static const uint8_t LED_PINS[LED_COUNT]  = { PIN_LED_RED, PIN_LED_GREEN, PIN_LED_BLUE, PIN_LED_YELLOW };
static const uint8_t LED_CHANS[LED_COUNT] = { 4, 5, 6, 7 };   // see PwmCompat.h
static bool autoMode = true;

static void raw(LedId led, uint8_t v) { pwmWrite(LED_PINS[led], LED_CHANS[led], v); }

void ledsBegin() {
    for (uint8_t i = 0; i < LED_COUNT; i++) {
        pwmAttach(LED_PINS[i], LED_CHANS[i], 5000, 8);
        raw((LedId)i, 0);
    }
}

void ledSet(LedId led, uint8_t brightness) {
    autoMode = false;
    raw(led, brightness);
}

void ledsAllOff()          { for (uint8_t i = 0; i < LED_COUNT; i++) raw((LedId)i, 0); }
void ledsSetAuto(bool on)  { autoMode = on; if (!on) ledsAllOff(); }
bool ledsAuto()            { return autoMode; }

// Automatic status display, refreshed ~50 times a second.
void ledsUpdate() {
    static uint32_t last = 0;
    if (!autoMode || millis() - last < 20) return;
    last = millis();

    // GREEN: slow "breathing" heartbeat = program is running
    float phase = (millis() % 3000) / 3000.0f;              // 0..1 every 3 s
    float wave  = 0.5f - 0.5f * cosf(phase * 2 * PI);       // 0..1 smooth
    raw(LED_GREEN, (uint8_t)(10 + wave * 120));

    // BLUE: a phone is connected (blinks while the web page is driving)
    bool blink = (millis() / 150) % 2;
    raw(LED_BLUE, webDriving() ? (blink ? 200 : 0) : (webClients() > 0 ? 60 : 0));

    // YELLOW: demo or line-follow running
    raw(LED_YELLOW, (demo.running() || follower.running()) ? 150 : 0);

    // RED: motors powered (driver awake and a motor is moving)
    bool moving = motors.enabled() && (motors.A.speed() != 0 || motors.B.speed() != 0);
    raw(LED_RED, moving ? 180 : 0);
}
