// =============================================================================
//  LineSensor.cpp  -  8-channel line sensor (Pololu QTR-8A) on J15, and a
//                     simple line-following demo (a "P controller").
//
//  PIN_LINE (pins.h) lists the sensors LEFT to RIGHT, and motor A is the LEFT
//  wheel. If your robot steers away from the line, check PIN_LINE's order,
//  or make FOLLOW_KP negative in config.h.
// =============================================================================
#include "Breakout.h"
#include "Log.h"
#include "App.h"
#include "config.h"
#include "pins.h"
#include <Preferences.h>

LineFollower follower;

// =============================================================================
//  Reading the sensors
//
//  The QTR-8A bar gives an ANALOG voltage per sensor: low (~0.2 V) over a
//  white floor, high (~3.3 V) over a black line or in the air. Pins that the
//  ESP32-S2 can read as analog (ADC: IO1-IO20, see pins.h) are read as
//  analog; any other pin is read as digital (just 0 or 1) - still useful to
//  check a channel is alive. Values are cached and refreshed ~200x a second.
//
//  Calibration ("line cal", or the web pages): for a few seconds, sweep the
//  bar over the line and the floor. Each sensor's darkest and lightest
//  readings are saved in flash, then every sensor is scaled to
//      0 = floor (white) ... 1000 = line (black)
//  so all 8 behave the same, and the line position becomes smooth.
// =============================================================================
static const uint16_t RAW_MAX = 4095;      // 12-bit readings
static bool     isAnalog[8];
static uint16_t raw[8];                    // latest raw reading 0..4095
static uint16_t calMin[8], calMax[8];      // lightest / darkest seen
static bool     calibrated = false;
static uint32_t calUntil = 0;              // non-zero = calibrating until then
static uint32_t lastSample = 0;

static void loadCal() {
    Preferences p;
    p.begin("line", true);
    calibrated = p.getBytes("min", calMin, sizeof(calMin)) == sizeof(calMin) &&
                 p.getBytes("max", calMax, sizeof(calMax)) == sizeof(calMax);
    p.end();
}

static void saveCal() {
    Preferences p;
    p.begin("line", false);
    p.putBytes("min", calMin, sizeof(calMin));
    p.putBytes("max", calMax, sizeof(calMax));
    p.end();
}

void lineBegin() {
    analogReadResolution(12);
    for (uint8_t i = 0; i < 8; i++) {
        uint8_t pin = PIN_LINE[i];
        // ADC-capable pins (not the USB pins 19/20) are read as analog
        isAnalog[i] = digitalPinToAnalogChannel(pin) >= 0 && pin != 19 && pin != 20;
        // No pull resistors: the QTR-8A has its own 47k pull-ups
        pinMode(pin, INPUT);
        if (isAnalog[i]) analogSetPinAttenuation(pin, ADC_11db);   // widest range (~0-2.5 V+)
    }
    loadCal();
}

static void sample() {
    for (uint8_t i = 0; i < 8; i++) {
        uint16_t v;
        if (isAnalog[i]) {
            // Average a few readings. Pins on ADC2 (IO11-IO20) share the ADC
            // with WiFi: a blocked read returns 0, so skip those.
            uint32_t sum = 0; uint8_t n = 0;
            for (uint8_t k = 0; k < LINE_SAMPLES; k++) {
                uint16_t r = analogRead(PIN_LINE[i]);
                if (r) { sum += r; n++; }
            }
            if (!n) continue;                      // keep the last good value
            v = sum / n;
        } else {
            v = digitalRead(PIN_LINE[i]) ? RAW_MAX : 0;
        }
        raw[i] = v;
        if (calUntil) {                            // calibrating: widen the range
            if (v < calMin[i]) calMin[i] = v;
            if (v > calMax[i]) calMax[i] = v;
        }
    }
}

static void sampleIfStale() {
    if (millis() - lastSample >= 5) { lastSample = millis(); sample(); }
}

// Call from loop(): keeps readings fresh and finishes a calibration.
void lineUpdate() {
    sampleIfStale();
    if (calUntil && (int32_t)(millis() - calUntil) >= 0) {
        calUntil = 0;
        calibrated = true;
        saveCal();
        Log.println(F("[line] calibration done:"));
        lineCalReport();
        displayMessage("Line calibrated", "see serial/web log");
    }
}

void lineCalStart(uint32_t ms) {
    for (uint8_t i = 0; i < 8; i++) { calMin[i] = RAW_MAX; calMax[i] = 0; }
    calUntil = millis() + ms;
    Log.printf("[line] calibrating for %lu s - sweep the sensor bar slowly back and forth\r\n"
               "       across the line so EVERY sensor sees both the line and the floor\r\n",
               (unsigned long)(ms / 1000));
    displayMessage("Calibrating line", "sweep over line");
}

bool lineCalibrating() { return calUntil != 0; }
bool lineCalibrated()  { return calibrated; }

void lineCalReset() {
    calibrated = false;
    Preferences p; p.begin("line", false); p.clear(); p.end();
    Log.println(F("[line] calibration cleared"));
}

// Per-channel summary: proves each pin and sensor is working.
void lineCalReport() {
    for (uint8_t i = 0; i < 8; i++) {
        int range = (int)calMax[i] - (int)calMin[i];
        const char* verdict = !calibrated ? "not calibrated"
                            : range > 1500 ? "OK"
                            : range > 400  ? "weak - check height / tape"
                            :                "NO CHANGE - check this sensor / pin";
        Log.printf("  %u from left  LF_%u  IO%-2u  %-7s  floor %4u  line %4u   %s\r\n", i + 1, LINE_LF_NUM[i],
                   PIN_LINE[i], isAnalog[i] ? "analog" : "digital", calMin[i], calMax[i], verdict);
    }
}

// 0 (floor) .. 1000 (line) for one sensor
uint16_t lineValue(uint8_t i) {
    sampleIfStale();
    if (i > 7) return 0;
    int32_t v;
    if (calibrated && calMax[i] > calMin[i] + 50)
        v = ((int32_t)raw[i] - calMin[i]) * 1000 / (calMax[i] - calMin[i]);
    else
        v = (int32_t)raw[i] * 1000 / RAW_MAX;           // uncalibrated: rough guess
    v = constrain(v, 0, 1000);
    return LINE_ACTIVE_HIGH ? v : 1000 - v;
}

uint16_t lineRaw(uint8_t i)      { sampleIfStale(); return i < 8 ? raw[i] : 0; }
bool     lineIsAnalog(uint8_t i) { return i < 8 && isAnalog[i]; }

// Which sensors see the line (value over LINE_THRESHOLD): bit 0 = LF_1
uint8_t lineRead() {
    uint8_t mask = 0;
    for (uint8_t i = 0; i < 8; i++) if (lineValue(i) > LINE_THRESHOLD) mask |= (1 << i);
    return mask;
}

// Smooth position from the 0..1000 values: -1 (far left) .. +1 (far right).
// A weighted average: darker sensors pull the answer towards them.
bool lineSmoothPosition(float& pos) {
    float sum = 0, total = 0;
    uint16_t best = 0;
    for (uint8_t i = 0; i < 8; i++) {
        uint16_t v = lineValue(i);
        if (v > best) best = v;
        if (v < 100) continue;                          // ignore noise near "floor"
        sum   += v * ((i - 3.5f) / 3.5f);
        total += v;
    }
    if (best < LINE_THRESHOLD || total == 0) return false;   // no line under the bar
    pos = sum / total;
    return true;
}

// Weighted average of the sensors that see the line (on/off version).
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

// e.g.  [..##....]  pos +0.12 |   12   30  980 1000   40    0    0    3
void linePrint(uint8_t mask) {
    char s[11] = "[........]";
    for (uint8_t i = 0; i < 8; i++) if (mask & (1 << i)) s[i + 1] = '#';
    float pos;
    if (lineSmoothPosition(pos)) Log.printf("%s  pos %+.2f |", s, pos);
    else                         Log.printf("%s  no line   |", s);
    for (uint8_t i = 0; i < 8; i++) Log.printf(" %4u", lineValue(i));
    Log.print(calibrated ? "\r\n" : "   (not calibrated: line cal)\r\n");
}

// --------------------------------------------------------------- LineFollower
void LineFollower::start() {
    stopAllModes();
    _savedRamp = motors.rampTime();
    motors.setRampTime(100);          // quick response for steering
    _running  = true;
    _lastSeen = millis();
    Log.println(F("[follow] started - put the robot on the line. SW2 or 'stop' to end"));
}

void LineFollower::stop() {
    if (!_running) return;
    _running = false;
    stopMotors(false);
    motors.setRampTime(_savedRamp);
    Log.println(F("[follow] stopped"));
}

void LineFollower::update() {
    if (!_running || millis() - _lastTick < 10) return;      // 100 times a second
    _lastTick = millis();

    float pos;
    if (lineSmoothPosition(pos)) {
        _lastSeen = millis();
        int steer = (int)(FOLLOW_KP * pos);                  // the "P" in PID
        driveMotors(FOLLOW_BASE_SPEED + steer, FOLLOW_BASE_SPEED - steer);
    } else if (millis() - _lastSeen > FOLLOW_LOST_MS) {
        Log.println(F("[follow] line lost"));
        stop();
    }
}
