// =============================================================================
//  Servos.cpp  -  Two hobby servos on J8 / J9 (powered from the BOB's 5 V).
//
//  A servo wants a pulse every 20 ms (50 Hz). The pulse WIDTH sets the angle:
//  about 0.5 ms = 0 degrees, 1.5 ms = 90 degrees, 2.5 ms = 180 degrees.
// =============================================================================
#include "Breakout.h"
#include "config.h"
#include "pins.h"
#include "PwmCompat.h"

static const uint8_t  SERVO_PINS[2]  = { PIN_SERVO_1, PIN_SERVO_2 };
static const uint8_t  SERVO_CHANS[2] = { 2, 3 };       // see PwmCompat.h
static const uint8_t  SERVO_BITS     = 14;             // 0..16383 per 20 ms
static const uint32_t PERIOD_US      = 20000;
static int angles[2] = { -1, -1 };

static void writeUs(uint8_t idx, uint32_t us) {
    uint32_t duty = (uint32_t)((uint64_t)us * ((1UL << SERVO_BITS) - 1) / PERIOD_US);
    pwmWrite(SERVO_PINS[idx], SERVO_CHANS[idx], duty);
}

void servosBegin() {
    for (uint8_t i = 0; i < 2; i++) {
        pwmAttach(SERVO_PINS[i], SERVO_CHANS[i], 50, SERVO_BITS);
        writeUs(i, 0);                 // no pulses until told to move
    }
}

void servoWrite(uint8_t num, int degrees) {
    if (num < 1 || num > 2) return;
    degrees = constrain(degrees, 0, 180);
    angles[num - 1] = degrees;
    writeUs(num - 1, map(degrees, 0, 180, SERVO_MIN_US, SERVO_MAX_US));
}

int servoAngle(uint8_t num) { return (num >= 1 && num <= 2) ? angles[num - 1] : -1; }

void servoOff(uint8_t num) {
    if (num < 1 || num > 2) return;
    angles[num - 1] = -1;
    writeUs(num - 1, 0);
}

void servoSweep(uint8_t num) {
    for (int a = 0; a <= 180; a += 5)  { servoWrite(num, a); delay(20); }
    for (int a = 180; a >= 90; a -= 5) { servoWrite(num, a); delay(20); }
}
