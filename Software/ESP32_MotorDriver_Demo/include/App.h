// =============================================================================
//  App.h  -  Things shared between the different parts of the program.
// =============================================================================
#pragma once
#include <Arduino.h>
#include <TB6612.h>

// The motor driver (defined in main.cpp)
extern TB6612 motors;

// Drive both motors (percent, -100..100). Wakes the driver if needed.
void driveMotors(int a, int b);

// Stop both motors. brake=true stops quickly, false lets them coast.
void stopMotors(bool brake = false);

// End the demo, line follower and web control (motors keep their targets
// unless a mode was running - follow with stopMotors() for a full stop).
void stopAllModes();

// Emergency stop: everything off and the driver put in standby.
void emergencyStop(const char* reason);

// Short text describing what the robot is doing ("Idle", "Demo", ...)
const char* modeName();

// Pretty-print chip / board information to Serial.
void printBoardInfo();

// ----------------------------------------------------------------- DemoRunner
// Plays a short "dance" on the two motors without needing a computer.
// Non-blocking: call demo.update() from loop().
class DemoRunner {
public:
    void start();
    void stop();
    void toggle() { running() ? stop() : start(); }
    bool running() const { return _running; }
    void update();
private:
    bool     _running = false;
    uint8_t  _step = 0;
    uint32_t _stepStart = 0;
    void     enterStep(uint8_t i);
};
extern DemoRunner demo;

// ------------------------------------------------------------------ Web page
void   webBegin(bool forceHotspot = false);  // start WiFi + web server
void   webHandle();         // call from loop()
void   webPrintStatus();    // IP address etc. to Serial
void   webNotifyStopped();  // tell web layer motors were stopped elsewhere
bool   webDriving();        // is the web page currently driving?
int    webClients();        // phones/laptops connected to our network
String webSsid();
String webAddress();
bool   webIsHotspot();      // true = our own network, false = joined a network
String webSavedSsid();      // saved network name ("" = none)
void   webSaveCredentials(const String& ssid, const String& pass);       // WPA2-Personal
void   webSaveEnterprise(const String& ssid, const String& user, const String& pass,
                         const String& ident, bool ttls);                // WPA2-Enterprise
void   webForgetCredentials();
void   webRebootSoon();     // restart in ~1.5 s (after replying)
void   webScanToSerial();   // list nearby networks on Serial

// --------------------------------------------------------------- Serial menu
void menuPrintHelp();
void menuPoll();            // call from loop()

// ------------------------------------------------------------------ Self test
void runSelfTest();         // interactive bring-up test over Serial
