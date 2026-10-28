// =============================================================================
//  SpeedControl.cpp  -  A PID speed loop for each wheel + a "drive straight"
//                       correction, using the motor encoders.
//
//  Every 20 ms, for each wheel:
//      measured = encoder counts moved / time, as % of top speed
//      error    = target - measured
//      power    = target            (feed-forward: a good first guess)
//               + Kp x error        (P: push harder the further off we are)
//               + Ki x sum(error)   (I: slowly removes any error that stays)
//               - Kd x change       (D: brakes the change, less overshoot)
//
//  Drive straight: while both wheels have the same target, add up how many
//  more counts wheel A has turned than wheel B. If A gets ahead, slow A a
//  little and speed B up a little (Ks) until they're level again.
// =============================================================================
#include <Preferences.h>
#include "SpeedControl.h"
#include "App.h"
#include "Breakout.h"
#include "config.h"

static const uint32_t TICK_MS = 20;               // 50 times a second

struct Wheel {
    int   target = 0;        // asked for, % of top speed
    float setpoint = 0;      // target after ramping
    float measured = 0;      // filtered actual speed, %
    float lastMeasured = 0;  // for the D term
    float integral = 0;      // I term (already x Ki, in % power)
    float output = 0;        // % power sent to the motor
    long  lastCount = 0;
    uint32_t badSince = 0;   // for the encoder fault check
};

static Wheel      w[2];
static bool       enabled = false;
static bool       logging = false;
static float      maxCps = SPEED_MAX_CPS;
static SpeedGains gains = { SPEED_KP, SPEED_KI, SPEED_KD, SPEED_KS };
static float      syncErr = 0;                    // counts A has gone further than B
static uint32_t   lastTick = 0;
static bool       idle = true;                    // nothing to control right now

static TB6612Motor& motor(uint8_t m) { return m == 0 ? motors.A : motors.B; }

// --------------------------------------------------------------- settings
void speedBegin() {
    Preferences p;
    p.begin("pid", true);
    enabled     = p.getBool("on", false);
    maxCps      = p.getFloat("max", SPEED_MAX_CPS);
    gains.kp    = p.getFloat("kp", SPEED_KP);
    gains.ki    = p.getFloat("ki", SPEED_KI);
    gains.kd    = p.getFloat("kd", SPEED_KD);
    gains.ks    = p.getFloat("ks", SPEED_KS);
    p.end();
    if (maxCps <= 0) enabled = false;
    for (uint8_t m = 0; m < 2; m++) w[m].lastCount = encoderCount(m);
}

void speedSaveGains() {
    Preferences p;
    p.begin("pid", false);
    p.putFloat("kp", gains.kp); p.putFloat("ki", gains.ki);
    p.putFloat("kd", gains.kd); p.putFloat("ks", gains.ks);
    p.putFloat("max", maxCps);
    p.end();
}

SpeedGains& speedGains()   { return gains; }
float speedMaxCps()        { return maxCps; }
bool  speedEnabled()       { return enabled; }
void  speedSetLogging(bool on) { logging = on; }
bool  speedLogging()       { return logging; }
int   speedTarget(uint8_t m)   { return m < 2 ? w[m].target : 0; }
float speedMeasured(uint8_t m) { return m < 2 ? w[m].measured : 0; }

bool speedEnable(bool on, bool save) {
    if (on && maxCps <= 0) {
        Log.println(F("[pid] not calibrated yet - wheels off the ground, then type: pid cal"));
        return false;
    }
    if (on != enabled) {
        // Hand over cleanly: carry the current targets across, start fresh
        int a = motorTarget(0), b = motorTarget(1);
        speedReset();
        enabled = on;
        if (a || b) driveMotors(a, b);
        Log.printf("[pid] speed control %s\r\n", on ? "ON (speeds are % of top speed)" : "OFF (speeds are % power)");
    }
    if (save) { Preferences p; p.begin("pid", false); p.putBool("on", on); p.end(); }
    return true;
}

void speedPrint() {
    Log.printf("Speed control (PID): %s\r\n", enabled ? "ON" : "off");
    if (maxCps > 0) Log.printf("  top speed  %.0f counts/s   (pid cal to re-measure)\r\n", maxCps);
    else            Log.println(F("  top speed  not measured - wheels off the ground, then: pid cal"));
    Log.printf("  kp %.2f   ki %.2f   kd %.3f   ks %.2f\r\n", gains.kp, gains.ki, gains.kd, gains.ks);
}

// ------------------------------------------------------------- targets
void speedSetTarget(int a, int b) {
    w[0].target = constrain(a, -100, 100);
    w[1].target = constrain(b, -100, 100);
    if (idle && (a || b)) {                       // starting from rest
        for (uint8_t m = 0; m < 2; m++) {
            w[m].lastCount = encoderCount(m);
            w[m].integral = 0;
            w[m].badSince = 0;
            w[m].measured = w[m].lastMeasured = 0;
        }
        syncErr = 0;
        lastTick = millis();
        idle = false;
    }
}

void speedReset() {
    for (auto& x : w) { x.target = 0; x.setpoint = 0; x.integral = 0; x.output = 0; x.badSince = 0; }
    syncErr = 0;
    idle = true;
}

// --------------------------------------------------------- the PID loop
void speedUpdate() {
    if (!enabled || idle) return;
    uint32_t now = millis();
    if (now - lastTick < TICK_MS) return;
    float dt = (now - lastTick) / 1000.0f;
    lastTick = now;

    if (!motors.enabled()) { speedReset(); return; }   // standby / emergency stop

    // 1. Ramp each setpoint towards its target (same ramp time as normal mode)
    float step = motors.rampTime() ? 100.0f * dt * 1000.0f / motors.rampTime() : 200.0f;
    long moved[2];
    for (uint8_t m = 0; m < 2; m++) {
        Wheel& x = w[m];
        float diff = x.target - x.setpoint;
        x.setpoint += constrain(diff, -step, step);

        // 2. Measure: counts this tick -> % of top speed (lightly filtered)
        long c = encoderCount(m);
        moved[m] = c - x.lastCount;
        x.lastCount = c;
        float pct = moved[m] / dt / maxCps * 100.0f;
        x.measured = 0.5f * x.measured + 0.5f * pct;
    }

    // 3. Drive straight: both wheels asked for the same speed and direction?
    bool straight = w[0].setpoint != 0 && fabsf(w[0].target - w[1].target) < 0.5f &&
                    fabsf(w[0].setpoint - w[1].setpoint) < 0.5f;
    float trim = 0;
    if (straight && gains.ks > 0) {
        syncErr += moved[0] - moved[1];
        float gapPct = syncErr / maxCps * 100.0f;          // gap in "% x seconds"
        trim = constrain(gains.ks * gapPct, -20.0f, 20.0f);
    } else {
        syncErr = 0;
    }

    // 4. PID for each wheel
    bool allStopped = true;
    for (uint8_t m = 0; m < 2; m++) {
        Wheel& x = w[m];
        float sp = x.setpoint + (m == 0 ? -trim : trim) / 2;
        if (x.target == 0 && fabsf(x.setpoint) < 0.01f) {  // this wheel is told to stop
            x.setpoint = 0; x.integral = 0; x.output = 0;
            if (!motor(m).isBraking()) motor(m).setOutput(0);
            continue;
        }
        allStopped = false;
        float err   = sp - x.measured;
        // D on the measurement (not the error) so a new target doesn't cause a kick
        float deriv = -gains.kd * (x.measured - x.lastMeasured) / dt;
        x.lastMeasured = x.measured;

        float out = sp + gains.kp * err + x.integral + deriv;
        // Anti-windup: don't integrate while the target is still ramping (the
        // wheel always lags a moving target - that would cause overshoot), or
        // while the output is maxed out (unless the error would bring it back).
        bool ramping = fabsf(x.target - x.setpoint) > 0.5f;
        if (!ramping && (fabsf(out) < 100.0f || (out > 0) != (err > 0))) {
            x.integral = constrain(x.integral + gains.ki * err * dt, -60.0f, 60.0f);
        }
        x.output = constrain(out, -100.0f, 100.0f);
        motor(m).setOutput(x.output);

        // 5. Safety: lots of power but the encoder says "not moving" or
        //    "moving backwards" -> encoder unplugged/reversed or wheel stalled.
        bool bad = fabsf(x.output) > 40 && (fabsf(x.measured) < 3 || (x.measured > 0) != (x.output > 0));
        if (!bad) x.badSince = 0;
        else if (!x.badSince) x.badSince = now;
        else if (now - x.badSince > 500) {
            char wheel = m == 0 ? 'A' : 'B';
            Log.printf("[pid] Motor %c: power %.0f%% but encoder reads %.0f%% - speed control OFF.\r\n"
                       "      Encoder unplugged, counting backwards (rev enc%c) or wheel stalled?\r\n",
                       wheel, x.output, x.measured, (char)tolower(wheel));
            displayMessage("PID: encoder?", m == 0 ? "motor A - pid off" : "motor B - pid off");
            stopAllModes();
            stopMotors(false);
            speedEnable(false, false);                  // off until re-enabled
            return;
        }
    }
    if (allStopped) { idle = true; }

    // Live values for tuning (shown in the serial monitor and web terminal)
    static uint32_t lastLog = 0;
    if (logging && now - lastLog >= 100) {
        lastLog = now;
        Log.printf("A sp %5.1f meas %5.1f out %5.1f | B sp %5.1f meas %5.1f out %5.1f | gap %+ld\r\n",
                   w[0].setpoint, w[0].measured, w[0].output,
                   w[1].setpoint, w[1].measured, w[1].output, (long)syncErr);
    }
}

// ---------------------------------------------------------- calibration
// Full power on both wheels for a moment and measure the counts per second.
// Top speed = the SLOWER wheel (x 0.9 headroom), so both can always reach 100 %.
bool speedCalibrate() {
    bool was = enabled;
    stopAllModes();
    stopMotors(false);
    enabled = false;
    Log.println(F("[pid] calibrating - both wheels full speed for 2 s (wheels OFF the ground!)"));
    displayMessage("PID calibrate", "wheels off ground!");
    motors.enable(true);
    motors.A.setOutput(100);
    motors.B.setOutput(100);
    uint32_t t0 = millis();
    while (millis() - t0 < 700) { webHandle(); delay(5); }       // spin up
    long a0 = encoderCount(0), b0 = encoderCount(1);
    t0 = millis();
    while (millis() - t0 < 1200) { webHandle(); delay(5); }
    float secs = (millis() - t0) / 1000.0f;
    float ca = (encoderCount(0) - a0) / secs, cb = (encoderCount(1) - b0) / secs;
    stopMotors(true);
    Log.printf("[pid] motor A %.0f counts/s, motor B %.0f counts/s\r\n", ca, cb);

    if (ca < 50 || cb < 50) {
        if (ca <= -50 || cb <= -50)
            Log.println(F("[pid] an encoder counts BACKWARDS - fix with 'rev enca' / 'rev encb' and try again"));
        else
            Log.println(F("[pid] an encoder isn't counting - check the encoder wiring, battery and STBY"));
        enabled = false;
        displayMessage("PID calibrate", "FAILED - see log");
        return false;
    }
    maxCps = min(ca, cb) * 0.9f;
    speedSaveGains();
    Log.printf("[pid] top speed set to %.0f counts/s (the slower wheel, less 10%%)\r\n", maxCps);
    if (fabsf(ca - cb) / max(ca, cb) > 0.15f)
        Log.println(F("[pid] note: the motors differ by more than 15% - speed control will even them up"));
    displayMessage("PID calibrate", "done");
    enabled = was;
    return true;
}
