// =============================================================================
//  Settings.cpp  -  Robot name + direction swaps, stored with Preferences (NVS).
//
//  Why "reverse" options?  If a motor's two wires are swapped (or the motors
//  face opposite ways on the chassis), "forward" makes it spin backwards.
//  Rather than re-crimp the cable, tick "reverse" and the code swaps it.
//  The same goes for encoders (counting backwards) and servos (mounted
//  the other way round, so 0 and 180 degrees are swapped).
// =============================================================================
#include <Preferences.h>
#include "Log.h"
#include "Settings.h"
#include "App.h"
#include "Breakout.h"
#include "config.h"

const char* const REV_NAMES[REV_COUNT] = { "motorA", "motorB", "encA", "encB", "servo1", "servo2" };
static const char* const REV_KEYS[REV_COUNT] = { "revMA", "revMB", "revEA", "revEB", "revS1", "revS2" };
static const bool REV_DEFAULTS[REV_COUNT] = { MOTOR_A_INVERTED, MOTOR_B_INVERTED,
                                              ENC_A_INVERTED, ENC_B_INVERTED,
                                              SERVO_1_INVERTED, SERVO_2_INVERTED };

static Preferences prefs;
static String hostname;          // saved name ("" = use the default)
static String password;          // saved password ("" = use the default)
static bool   rev[REV_COUNT];

// Push the current reverse settings into the drivers
static void apply(RevItem i) {
    switch (i) {
        case REV_MOTOR_A: motors.A.setInverted(rev[i]); break;
        case REV_MOTOR_B: motors.B.setInverted(rev[i]); break;
        case REV_ENC_A:   encoderSetInverted(0, rev[i]); break;
        case REV_ENC_B:   encoderSetInverted(1, rev[i]); break;
        case REV_SERVO_1: servoSetInverted(1, rev[i]); break;
        case REV_SERVO_2: servoSetInverted(2, rev[i]); break;
        default: break;
    }
}

void settingsBegin() {
    prefs.begin("cfg", true);                        // read-only
    hostname = prefs.getString("host", "");
    password = prefs.getString("pw", "");
    for (uint8_t i = 0; i < REV_COUNT; i++) rev[i] = prefs.getBool(REV_KEYS[i], REV_DEFAULTS[i]);
    prefs.end();
    for (uint8_t i = 0; i < REV_COUNT; i++) apply((RevItem)i);
}

// ------------------------------------------------------------------ name
String settingsHostname() {
    if (hostname.length()) return hostname;
    uint64_t mac = ESP.getEfuseMac();                // default: motordriver-1a2b
    char suffix[6];
    snprintf(suffix, sizeof(suffix), "-%02x%02x", (uint8_t)(mac >> 32), (uint8_t)(mac >> 40));
    return String(DEFAULT_HOSTNAME) + suffix;
}

bool settingsHostnameCustom() { return hostname.length() > 0; }

// Hostnames may only use a-z, 0-9 and '-', and can't start or end with '-'.
bool settingsValidHostname(const String& name, String* why) {
    auto fail = [&](const char* msg) { if (why) *why = msg; return false; };
    if (name.isEmpty())        return fail("Name is empty");
    if (name.length() > 24)    return fail("Name is too long (24 characters max)");
    if (name[0] == '-' || name[name.length() - 1] == '-')
                               return fail("Name can't start or end with '-'");
    for (char c : name)
        if (!(isdigit(c) || (c >= 'a' && c <= 'z') || c == '-'))
            return fail("Use only letters a-z, numbers 0-9 and '-' (no spaces)");
    return true;
}

bool settingsSetHostname(String name, String* why) {
    name.trim();
    name.toLowerCase();
    if (name.length() && !settingsValidHostname(name, why)) return false;
    hostname = name;
    prefs.begin("cfg", false);
    prefs.putString("host", hostname);
    prefs.end();
    return true;
}

// -------------------------------------------------------------- password
String settingsPassword()       { return password.length() ? password : String(ROBOT_PASSWORD); }
bool   settingsPasswordCustom() { return password.length() > 0; }

bool settingsSetPassword(String pw, String* why) {
    auto fail = [&](const char* msg) { if (why) *why = msg; return false; };
    if (pw.length()) {
        if (pw.length() < 8)  return fail("Password must be at least 8 characters (a WiFi rule)");
        if (pw.length() > 32) return fail("Password is too long (32 characters max)");
        for (char c : pw)
            if (c < 33 || c > 126 || c == '"' || c == '\\')
                return fail("Use letters, numbers and symbols - no spaces, quotes or backslashes");
    }
    password = pw;
    Preferences p;
    p.begin("cfg", false);
    p.putString("pw", password);
    p.end();
    return true;
}

// --------------------------------------------------------------- reverse
bool settingsReversed(RevItem item) { return item < REV_COUNT && rev[item]; }

void settingsSetReversed(RevItem item, bool reversed) {
    if (item >= REV_COUNT) return;
    rev[item] = reversed;
    prefs.begin("cfg", false);
    prefs.putBool(REV_KEYS[item], reversed);
    prefs.end();
    apply(item);
    Log.printf("[cfg] %s %s\r\n", REV_NAMES[item], reversed ? "REVERSED" : "normal");
}

int settingsRevFromName(const String& nameIn) {
    String n = nameIn; n.toLowerCase();
    if (n == "a" || n == "motora")                return REV_MOTOR_A;
    if (n == "b" || n == "motorb")                return REV_MOTOR_B;
    if (n == "enca")                              return REV_ENC_A;
    if (n == "encb")                              return REV_ENC_B;
    if (n == "s1" || n == "servo1" || n == "1")   return REV_SERVO_1;
    if (n == "s2" || n == "servo2" || n == "2")   return REV_SERVO_2;
    return -1;
}
