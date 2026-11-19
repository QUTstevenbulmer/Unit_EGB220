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
//  white surface, high (~3.3 V) over a black surface or in the air. Pins that the
//  ESP32-S2 can read as analog (ADC: IO1-IO20, see pins.h) are read as
//  analog; any other pin is read as digital (just 0 or 1) - still useful to
//  check a channel is alive. Values are cached and refreshed ~200x a second.
//
//  Calibration ("line cal", or the web pages): for a few seconds, sweep the
//  bar over the line and the floor. Each sensor's darkest and lightest
//  readings are saved in flash, then every sensor is scaled to
//      0 = floor ... 1000 = line   (LINE_ACTIVE_HIGH in config.h says
//      whether the line is the white or the black surface)
//  so all 8 behave the same, and the line position becomes smooth.
// =============================================================================
static const uint16_t RAW_MAX = 4095;      // 12-bit readings
static bool     isAnalog[8];
static uint16_t raw[8];                    // latest (filtered) reading 0..4095
static float    filt[8];                   // smoothed readings
static bool     sees[8];                   // "sees the line", with hysteresis
static uint16_t calMin[8], calMax[8];      // lightest / darkest seen
static bool     calibrated = false;
static uint32_t calUntil = 0;              // non-zero = calibrating until then
static uint32_t lastSample = 0;

// The calibration is stored with the pin list it was made with. If PIN_LINE
// changes (new board revision, bar flipped...), the old calibration belongs
// to different sensors, so it's thrown away instead of scrambling readings.
static void loadCal() {
    Preferences p;
    p.begin("line", true);
    uint8_t pins[8] = { 0 };
    bool samePins = p.getBytes("pins", pins, sizeof(pins)) == sizeof(pins) &&
                    memcmp(pins, PIN_LINE, sizeof(pins)) == 0;
    calibrated = samePins &&
                 p.getBytes("min", calMin, sizeof(calMin)) == sizeof(calMin) &&
                 p.getBytes("max", calMax, sizeof(calMax)) == sizeof(calMax);
    bool hadOld = !samePins && p.isKey("min");
    p.end();
    if (hadOld) Log.println(F("[line] pin map changed since the last calibration - please run: line cal"));
}

static void saveCal() {
    Preferences p;
    p.begin("line", false);
    p.putBytes("min", calMin, sizeof(calMin));
    p.putBytes("max", calMax, sizeof(calMax));
    p.putBytes("pins", PIN_LINE, sizeof(PIN_LINE));
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

// One clean reading of an analog pin.
//  * The ESP32 has one ADC shared by all pins. Its sampling capacitor still
//    holds some of the PREVIOUS pin's voltage, and the QTR-8A's 47k outputs
//    are slow to correct it - so the first reading after switching pins is
//    thrown away ("crosstalk" fix).
//  * Then take a few readings and use the MIDDLE one (median): one odd spike
//    from WiFi or motor noise can't drag it the way it would an average.
//  * ADC2 pins (IO11-IO20) can be blocked by WiFi: that returns 0, skip it.
static bool readAnalog(uint8_t pin, uint16_t& out) {
    uint16_t r[9]; uint8_t n = 0;
    analogRead(pin);                                   // settle (discard)
    for (uint8_t k = 0; k < LINE_SAMPLES && k < 9; k++) {
        uint16_t v = analogRead(pin);
        if (v) r[n++] = v;
    }
    if (!n) return false;
    for (uint8_t a = 1; a < n; a++)                    // tiny insertion sort
        for (uint8_t b = a; b > 0 && r[b] < r[b - 1]; b--) { uint16_t t = r[b]; r[b] = r[b - 1]; r[b - 1] = t; }
    out = r[n / 2];
    return true;
}

static void sample() {
    for (uint8_t i = 0; i < 8; i++) {
        uint16_t v;
        if (isAnalog[i]) {
            if (!readAnalog(PIN_LINE[i], v)) continue;     // keep the last good value
            // Light smoothing (time constant ~25 ms): steadies the bars
            // without slowing the line follower noticeably
            filt[i] += (v - filt[i]) * LINE_SMOOTHING;
            v = (uint16_t)(filt[i] + 0.5f);
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
    if (millis() - lastSample >= 10) { lastSample = millis(); sample(); }   // 100x a second
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
    Log.printf("[line] calibrating for %lu s - EVERY sensor must see both surfaces:\r\n"
               "       hold the whole bar over the line colour for a few seconds, then the whole\r\n"
               "       bar over the floor colour (or slide sideways so the line passes under all 8)\r\n",
               (unsigned long)(ms / 1000));
    displayMessage("Calibrating line", "line, then floor");
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
                            : range > 400  ? "weak - check height, or didn't see both surfaces"
                            :                "NO CHANGE - didn't see both surfaces, or check sensor / pin";
        Log.printf("  %u from left  LF_%u  IO%-2u  %-7s  light %4u  dark %4u   %s\r\n", i + 1, LINE_LF_NUM[i],
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

// Which sensors see the line: bit 0 = left-most sensor.
// Hysteresis: a sensor switches ON above THRESHOLD + HYSTERESIS and OFF below
// THRESHOLD - HYSTERESIS, so one hovering near the edge doesn't flicker.
uint8_t lineRead() {
    uint8_t mask = 0;
    for (uint8_t i = 0; i < 8; i++) {
        uint16_t v = lineValue(i);
        if (v > LINE_THRESHOLD + LINE_HYSTERESIS) sees[i] = true;
        else if (v < LINE_THRESHOLD - LINE_HYSTERESIS) sees[i] = false;
        if (sees[i]) mask |= (1 << i);
    }
    return mask;
}

// Diagnostics: hold the bar still, then measure each channel for 1 s.
// Big "noise" (max - min) on one channel = a bad joint, a wire picking up
// noise, or a sensor sitting right at the edge of the line.
void lineNoiseReport() {
    uint16_t mn[8], mx[8]; uint32_t sum[8] = { 0 }; uint16_t n = 0;
    for (uint8_t i = 0; i < 8; i++) { mn[i] = RAW_MAX; mx[i] = 0; }
    Log.println(F("[line] measuring noise for 1 s - keep the bar still..."));
    uint32_t t0 = millis();
    while (millis() - t0 < 1000) {
        for (uint8_t i = 0; i < 8; i++) {
            uint16_t v;
            if (isAnalog[i]) { analogRead(PIN_LINE[i]); v = analogRead(PIN_LINE[i]); }
            else v = digitalRead(PIN_LINE[i]) ? RAW_MAX : 0;
            if (v < mn[i]) mn[i] = v;
            if (v > mx[i]) mx[i] = v;
            sum[i] += v;
        }
        n++;
        delay(2);
    }
    Log.println(F("  sensor          GPIO  average   min   max  noise"));
    for (uint8_t i = 0; i < 8; i++) {
        uint16_t pp = mx[i] - mn[i];
        Log.printf("  %u from left LF_%u  IO%-2u  %5lu  %5u %5u  %5u %s\r\n", i + 1, LINE_LF_NUM[i], PIN_LINE[i],
                   (unsigned long)(sum[i] / n), mn[i], mx[i], pp,
                   pp > 400 ? "<- noisy" : pp > 150 ? "<- a bit noisy" : "");
    }
    Log.println(F("  (raw 0..4095: about 0.8 mV per step. Under ~150 = steady.)"));
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
