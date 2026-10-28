// =============================================================================
//  ESP32-S2 Motor Driver + Breakout Board - demo & hardware test firmware
//
//  What it does:
//    * USB serial menu   - type "help" in the PlatformIO serial monitor
//    * WiFi web page     - drive motors, servos and LEDs from a phone
//    * OLED              - live status; BOOT button flips pages
//    * Buttons           - SW1: motor demo   SW2: line follow
//                          hold ANY button 1 s: emergency stop
//    * Status LEDs       - green breathing = running, blue = phone connected,
//                          yellow = demo/follow, red = motors powered
//    * Self test         - type "selftest" for a guided board bring-up
//
//  File guide:
//    include/pins.h        every GPIO (controller + breakout)
//    include/config.h      settings to play with
//    lib/TB6612/           the motor driver library
//    src/main.cpp          setup(), loop(), modes, demo       <- you are here
//    src/SerialMenu.cpp    text commands
//    src/WebUI.cpp         WiFi + web server (page in WebPage.h)
//    src/SelfTest.cpp      guided hardware test
//    src/Leds.cpp, Buttons.cpp, Servos.cpp, Encoders.cpp,
//    src/LineSensor.cpp, Display.cpp     breakout board peripherals
//    src/HeaderIO.cpp      raw header pin tests + I2C scan
//    src/Settings.cpp      robot name + reverse options (saved in flash)
//    src/SpeedControl.cpp  encoder speed control (PID) - drive straight
// =============================================================================
#include <Arduino.h>
#include "Log.h"
#include <Wire.h>
#include "pins.h"
#include "config.h"
#include "App.h"
#include "Breakout.h"
#include "Settings.h"
#include "SpeedControl.h"

TB6612     motors;
DemoRunner demo;

// --------------------------------------------------------------- motor helpers
void driveMotors(int a, int b) {
    if (!motors.enabled()) motors.enable(true);
    if (speedEnabled()) {
        speedSetTarget(a, b);          // % of top speed - the PID loop sets the power
    } else {
        motors.A.setTarget(a);         // % of battery power (ramped)
        motors.B.setTarget(b);
    }
}

int motorTarget(uint8_t m) {
    if (speedEnabled()) return speedTarget(m);
    return m == 0 ? motors.A.target() : motors.B.target();
}

void stopMotors(bool brake) {
    speedReset();
    motors.stopAll(brake);
}

void stopAllModes() {
    demo.stop();
    follower.stop();
    webNotifyStopped();
}

void emergencyStop(const char* reason) {
    stopAllModes();
    stopMotors(true);
    motors.enable(false);
    Log.printf("[STOP] %s - motors in standby\r\n", reason);
    displayMessage("EMERGENCY STOP", reason);
}

const char* modeName() {
    if (demo.running())     return "Demo";
    if (follower.running()) return "Line follow";
    if (webDriving())       return "Web control";
    if (motorTarget(0) || motorTarget(1)) return "Manual";
    return "Idle";
}

// ------------------------------------------------------------------ board info
void printBoardInfo() {
    uint64_t mac = ESP.getEfuseMac();
    Log.println();
    Log.println(F("=================================================="));
    Log.println(F("  ESP32-S2 Motor Driver + BOB  -  demo firmware v" FW_VERSION));
    Log.println(F("=================================================="));
    Log.printf("  Chip       : %s rev %d, %u MHz\r\n",
                  ESP.getChipModel(), ESP.getChipRevision(), ESP.getCpuFreqMHz());
    Log.printf("  Flash      : %u MB    Free heap: %u bytes\r\n",
                  ESP.getFlashChipSize() / (1024 * 1024), ESP.getFreeHeap());
    Log.printf("  MAC        : %02X:%02X:%02X:%02X:%02X:%02X\r\n",
                  (uint8_t)(mac), (uint8_t)(mac >> 8), (uint8_t)(mac >> 16),
                  (uint8_t)(mac >> 24), (uint8_t)(mac >> 32), (uint8_t)(mac >> 40));
    Log.printf("  Chip temp  : %.1f C\r\n", temperatureRead());
    Log.printf("  Motors     : %s, ramp %lu ms, PWM %d Hz\r\n",
                  motors.enabled() ? "enabled" : "standby",
                  (unsigned long)motors.rampTime(), MOTOR_PWM_FREQ_HZ);
    Log.printf("  OLED       : %s\r\n", displayOk() ? "found" : "NOT found (check BOB / I2C)");
    Log.printf("  Password   : %s\r\n", settingsPasswordCustom() ? "set by you (web login, hotspot, OTA)"
                                       : "DEFAULT \"" ROBOT_PASSWORD "\" - set your own: password <new>");
    Log.printf("  Robot name : %s%s\r\n", settingsHostname().c_str(),
                  settingsHostnameCustom() ? "" : "  (default - change with: name <newname>)");
    String r;
    for (uint8_t i = 0; i < REV_COUNT; i++)
        if (settingsReversed((RevItem)i)) r += String(REV_NAMES[i]) + " ";
    Log.printf("  Reversed   : %s\r\n", r.length() ? r.c_str() : "nothing");
    Log.printf("  Speed ctrl : %s\r\n", speedEnabled() ? "PID on (speeds = % of top speed)"
                                         : speedMaxCps() > 0 ? "off (pid on to use it)" : "off, not calibrated (pid cal)");
}

// ---------------------------------------------------------------- motor demo
// Each step: speed for motor A, motor B (percent), how long, brake at end?
struct DemoStep { int a; int b; uint16_t ms; bool brake; const char* name; };

static const DemoStep DEMO_STEPS[] = {
    {  60,  60, 2000, false, "forward"       },
    {   0,   0,  800, false, "coast"         },
    { -60, -60, 2000, false, "reverse"       },
    {   0,   0,  800, false, "coast"         },
    {  60, -60, 1500, false, "spin right"    },
    { -60,  60, 1500, false, "spin left"     },
    { 100, 100, 1500, true,  "full + brake"  },
    {   0,   0, 1500, false, "pause"         },
};
static const uint8_t DEMO_STEP_COUNT = sizeof(DEMO_STEPS) / sizeof(DEMO_STEPS[0]);

void DemoRunner::start() {
    stopAllModes();
    _running = true;
    Log.println(F("[demo] started - SW1 or 'stop' to end"));
    enterStep(0);
}

void DemoRunner::stop() {
    if (!_running) return;
    _running = false;
    stopMotors(false);
    Log.println(F("[demo] stopped"));
}

void DemoRunner::enterStep(uint8_t i) {
    _step = i;
    _stepStart = millis();
    const DemoStep& s = DEMO_STEPS[i];
    Log.printf("[demo] %s  (A=%d%%, B=%d%%)\r\n", s.name, s.a, s.b);
    driveMotors(s.a, s.b);
    // Wave the servos along with the motors, just for fun
    servoWrite(1, 90 + s.a * 0.8);
    servoWrite(2, 90 + s.b * 0.8);
}

void DemoRunner::update() {
    if (!_running) return;
    const DemoStep& s = DEMO_STEPS[_step];
    if (millis() - _stepStart >= s.ms) {
        if (s.brake) stopMotors(true);
        enterStep((_step + 1) % DEMO_STEP_COUNT);
    }
}

// --------------------------------------------------------------- the buttons
static void handleButtons() {
    buttonsUpdate();

    // Long press on any button = emergency stop
    for (uint8_t b = 0; b < BTN_COUNT; b++)
        if (buttonLongPressed((ButtonId)b)) emergencyStop("button held");

    if (buttonPressed(BTN_SW1)) {
        demo.toggle();
        displayMessage(demo.running() ? "Demo started" : "Demo stopped", "SW1 to toggle");
    }
    if (buttonPressed(BTN_SW2)) {
        follower.toggle();
        displayMessage(follower.running() ? "Line follow ON" : "Line follow OFF", "SW2 to toggle");
    }
    if (buttonPressed(BTN_BOOT)) displayNextPage();
}

// ======================================================================= setup
void setup() {
    // Motors first, so they are guaranteed OFF while everything else starts.
    motors.begin(PIN_MOTOR_STBY);
    motors.A.begin(PIN_MOTOR_A_PWM, PIN_MOTOR_A_IN1, PIN_MOTOR_A_IN2, 0,
                   MOTOR_PWM_FREQ_HZ, MOTOR_PWM_BITS);
    motors.B.begin(PIN_MOTOR_B_PWM, PIN_MOTOR_B_IN1, PIN_MOTOR_B_IN2, 1,
                   MOTOR_PWM_FREQ_HZ, MOTOR_PWM_BITS);
    motors.setRampTime(MOTOR_RAMP_MS);

    Serial.begin(115200);                    // USB CDC - any baud rate works

    // Breakout board
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000);
    ledsBegin();
    buttonsBegin();
    servosBegin();
    encodersBegin();
    lineBegin();
    displayBegin();
    settingsBegin();             // robot name + reverse options saved in flash
    speedBegin();                // encoder speed control (PID) settings

    // Hold SW2 while powering on = robot password back to the default
    if (digitalRead(PIN_SW2) == LOW) {
        settingsSetPassword("");
        Log.println(F("[cfg] SW2 held at power-up - robot password reset to the default"));
        displayMessage("Password reset", ROBOT_PASSWORD);
        delay(1500);
    }

    // Hold SW1 while powering on = forget the saved WiFi network
    bool forceHotspot = digitalRead(PIN_SW1) == LOW;
    webBegin(forceHotspot);
}

// ======================================================================== loop
void loop() {
    // Print the banner whenever a serial monitor connects.
    static bool usbWasConnected = false;
    bool usbConnected = (bool)Serial;
    if (usbConnected && !usbWasConnected) {
        delay(100);
        printBoardInfo();
        webPrintStatus();
        menuPrintHelp();
    }
    usbWasConnected = usbConnected;

    menuPoll();
    webHandle();
    handleButtons();
    demo.update();
    follower.update();
    speedUpdate();               // PID speed loop (if turned on)
    motors.update();
    encodersUpdate();
    ledsUpdate();
    displayUpdate();
    delay(1);
}
