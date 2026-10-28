// =============================================================================
//  Settings.h  -  Per-robot settings kept in flash (survive re-uploading code).
//
//  Change them from the web page ("Robot setup" card) or the serial monitor:
//      name <hostname>      rev a | rev b | rev enca | rev encb | rev s1 | rev s2
//  The defaults come from config.h.
// =============================================================================
#pragma once
#include <Arduino.h>

enum RevItem : uint8_t { REV_MOTOR_A, REV_MOTOR_B, REV_ENC_A, REV_ENC_B,
                         REV_SERVO_1, REV_SERVO_2, REV_COUNT };
extern const char* const REV_NAMES[REV_COUNT];   // "motorA", "motorB", ...

void   settingsBegin();               // load from flash and apply (call early in setup)

// Robot name: used for the hotspot name AND http://<name>.local
String settingsHostname();            // the name in use (saved, or default + MAC)
bool   settingsHostnameCustom();      // true if a name was saved
bool   settingsValidHostname(const String& name, String* why = nullptr);
bool   settingsSetHostname(String name, String* why = nullptr);  // "" = back to default
                                      // (takes effect after a reboot)

// Robot password: web login, hotspot and firmware updates (see config.h)
String settingsPassword();
bool   settingsPasswordCustom();      // true if the team has set its own
bool   settingsSetPassword(String pw, String* why = nullptr);   // "" = back to default

// Direction swaps - take effect immediately
bool   settingsReversed(RevItem item);
void   settingsSetReversed(RevItem item, bool reversed);
int    settingsRevFromName(const String& name);  // "a", "motorA", "s1"... -> RevItem, -1 if unknown
