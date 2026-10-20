// =============================================================================
//  Breakout.h  -  Peripherals on the breakout board (BOB).
//
//      Leds.cpp        4 status LEDs (PWM brightness, automatic status mode)
//      Buttons.cpp     SW1 / SW2 / BOOT with debouncing
//      Servos.cpp      2 hobby servos on J8 / J9
//      Encoders.cpp    quadrature encoders on the motor connectors J5 / J6
//      LineSensor.cpp  8-channel line sensor on J15 + line-follow demo
//      Display.cpp     0.91" 128x32 OLED
// =============================================================================
#pragma once
#include <Arduino.h>

// ------------------------------------------------------------------- LEDs
enum LedId : uint8_t { LED_RED, LED_GREEN, LED_BLUE, LED_YELLOW, LED_COUNT };
extern const char* const LED_NAMES[LED_COUNT];

void ledsBegin();
void ledSet(LedId led, uint8_t brightness);   // 0..255 (turns auto mode off)
void ledsAllOff();
void ledsSetAuto(bool on);                    // automatic status display
bool ledsAuto();
void ledsUpdate();                            // call from loop()
//  Auto mode:  GREEN breathes = running      BLUE  = phone connected
//              YELLOW = demo/line-follow     RED   = motors powered

// ---------------------------------------------------------------- Buttons
enum ButtonId : uint8_t { BTN_SW1, BTN_SW2, BTN_BOOT, BTN_COUNT };

void buttonsBegin();
void buttonsUpdate();                         // call from loop()
bool buttonDown(ButtonId b);                  // held right now?
bool buttonPressed(ButtonId b);               // short press happened (clears)
bool buttonLongPressed(ButtonId b);           // held > 1 s (clears)

// ----------------------------------------------------------------- Servos
void servosBegin();
void servoWrite(uint8_t num, int degrees);    // num = 1 or 2, 0..180
int  servoAngle(uint8_t num);                 // last angle, -1 = off
void servoOff(uint8_t num);                   // stop pulses (servo goes limp)
void servoSweep(uint8_t num);                 // 0 -> 180 -> 90 (blocking ~2 s)

// --------------------------------------------------------------- Encoders
void  encodersBegin();
void  encodersUpdate();                       // call from loop()
long  encoderCount(uint8_t motor);            // 0 = A, 1 = B
void  encodersReset();
float encoderRate(uint8_t motor);             // RPM if ENC_COUNTS_PER_REV set,
                                              // otherwise counts per second
const char* encoderRateUnit();

// ------------------------------------------------------------ Line sensor
void    lineBegin();
uint8_t lineRead();                           // bit 0 = LF_1 ... bit 7 = LF_8
bool    linePosition(uint8_t mask, float& pos); // -1 (left) .. +1 (right)
void    linePrint(uint8_t mask);              // e.g.  [..##....]

class LineFollower {
public:
    void start();
    void stop();
    void toggle() { running() ? stop() : start(); }
    bool running() const { return _running; }
    void update();                            // call from loop()
private:
    bool     _running = false;
    uint32_t _lastSeen = 0;
    uint32_t _lastTick = 0;
    uint32_t _savedRamp = 0;
};
extern LineFollower follower;

// ---------------------------------------------------------------- Display
bool displayBegin();
bool displayOk();
void displayUpdate();                         // call from loop()
void displayNextPage();
void displayMessage(const char* line1, const char* line2 = "");
