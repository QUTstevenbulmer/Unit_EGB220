// =============================================================================
//  App.h  -  Things shared between the different parts of the program.
// =============================================================================
#pragma once
#include <Arduino.h>
#include <TB6612.h>
#include "Log.h"          // Log.printf(...) = Serial + web terminal

// The motor driver (defined in main.cpp)
extern TB6612 motors;

// Drive both motors (percent, -100..100). Wakes the driver if needed.
void driveMotors(int a, int b);

// What the motors were last asked for (percent). With speed control on this
// is % of top speed, otherwise % power.
int motorTarget(uint8_t motor);

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
// Saved networks (up to WIFI_MAX_NETWORKS, tried in list order at power-up)
String  webAddNetwork(String ssid, const String& pass, bool enterprise = false,
                      String user = "", String ident = "", bool ttls = false);
                            // returns "" if saved, otherwise what's wrong
uint8_t webNetworkCount();
String  webNetworkSsid(uint8_t i);
bool    webNetworkEnt(uint8_t i);
bool    webRemoveNetwork(uint8_t i);
bool    webNetworkToTop(uint8_t i);   // tried first
void    webForgetCredentials();       // forget them all
void   webRebootSoon();     // restart in ~1.5 s (after replying)
void   webScanToSerial();   // list nearby networks on Serial
bool   webUpdating();       // over-the-air update in progress

// --------------------------------------------------------------- Serial menu
void menuPrintHelp();
void menuPoll();            // call from loop()
void menuRunCommand(const String& text);   // command from the web terminal

// ------------------------------------------------------------------ Self test
void runSelfTest();         // interactive bring-up test over Serial
