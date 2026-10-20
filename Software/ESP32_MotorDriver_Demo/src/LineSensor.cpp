// =============================================================================
//  LineSensor.cpp  -  8-channel digital line sensor on J15, and a simple
//                     line-following demo (a "P controller").
//
//  Sensor 1 (LF_1) is assumed to be on the LEFT of the robot, sensor 8 on the
//  right, and motor A is the LEFT wheel. If your robot steers away from the
//  line, either flip the sensor bar or make FOLLOW_KP negative in config.h.
// =============================================================================
#include "Breakout.h"
#include "App.h"
#include "config.h"
#include "pins.h"

LineFollower follower;

void lineBegin() {
    // Pull-downs: with no sensor plugged in every channel reads "no line",
    // so the line-follow demo can't run off by accident.
    for (uint8_t i = 0; i < 8; i++) pinMode(PIN_LINE[i], INPUT_PULLDOWN);
}

uint8_t lineRead() {
    uint8_t mask = 0;
    for (uint8_t i = 0; i < 8; i++) {
        bool high = digitalRead(PIN_LINE[i]) == HIGH;
        if (high == LINE_ACTIVE_HIGH) mask |= (1 << i);
    }
    return mask;
}

// Weighted average of the sensors that see the line.
// Sensor positions: -3.5 -2.5 ... +2.5 +3.5  ->  scaled to -1 .. +1
bool linePosition(uint8_t mask, float& pos) {
    if (mask == 0) return false;
    float sum = 0; int n = 0;
    for (uint8_t i = 0; i < 8; i++) {
        if (mask & (1 << i)) { sum += i - 3.5f; n++; }
    }
    pos = (sum / n) / 3.5f;
    return true;
}

void linePrint(uint8_t mask) {
    char s[11] = "[........]";
    for (uint8_t i = 0; i < 8; i++) if (mask & (1 << i)) s[i + 1] = '#';
    float pos;
    if (linePosition(mask, pos)) Serial.printf("%s  position %+.2f\n", s, pos);
    else                         Serial.printf("%s  no line\n", s);
}

// --------------------------------------------------------------- LineFollower
void LineFollower::start() {
    stopAllModes();
    _savedRamp = motors.rampTime();
    motors.setRampTime(100);          // quick response for steering
    _running  = true;
    _lastSeen = millis();
    Serial.println(F("[follow] started - put the robot on the line. SW2 or 'stop' to end"));
}

void LineFollower::stop() {
    if (!_running) return;
    _running = false;
    stopMotors(false);
    motors.setRampTime(_savedRamp);
    Serial.println(F("[follow] stopped"));
}

void LineFollower::update() {
    if (!_running || millis() - _lastTick < 10) return;      // 100 times a second
    _lastTick = millis();

    float pos;
    if (linePosition(lineRead(), pos)) {
        _lastSeen = millis();
        int steer = (int)(FOLLOW_KP * pos);                  // the "P" in PID
        driveMotors(FOLLOW_BASE_SPEED + steer, FOLLOW_BASE_SPEED - steer);
    } else if (millis() - _lastSeen > FOLLOW_LOST_MS) {
        Serial.println(F("[follow] line lost"));
        stop();
    }
}
