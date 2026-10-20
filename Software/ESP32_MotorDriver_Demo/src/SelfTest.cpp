// =============================================================================
//  SelfTest.cpp  -  Guided bring-up test for the controller + breakout board.
//
//  Type "selftest" in the serial monitor. Each step says what to look for and
//  asks  y = pass,  n = fail,  s = skip.  Some steps check themselves.
//  A summary prints at the end.
//
//  Before starting: controller plugged into the breakout, encoder motors on
//  J5 / J6, battery on the XT30 - and the wheels OFF the table!
// =============================================================================
#include <Arduino.h>
#include "App.h"
#include "Breakout.h"
#include "HeaderIO.h"
#include "pins.h"

enum Result : uint8_t { T_PASS, T_FAIL, T_SKIP };

struct TestRecord { const char* name; Result r; };
static const uint8_t MAX_RESULTS = 20;
static TestRecord results[MAX_RESULTS];
static uint8_t    resultCount = 0;

static void record(const char* name, Result r) {
    if (resultCount < MAX_RESULTS) results[resultCount++] = { name, r };
}

// Keep the background jobs running while a test waits.
static void tick() {
    motors.update(); encodersUpdate(); buttonsUpdate(); displayUpdate();
    delay(5);
}

static void waitMs(uint32_t ms) {
    uint32_t start = millis();
    while (millis() - start < ms) tick();
}

// Wait for y / n / s.
static Result ask(const char* question) {
    while (Serial.available()) Serial.read();          // clear old input
    Serial.printf("   -> %s  [y/n/s] ", question);
    uint32_t start = millis();
    while (millis() - start < 60000) {
        tick();
        if (Serial.available()) {
            char c = tolower(Serial.read());
            if (c == 'y') { Serial.println("PASS"); return T_PASS; }
            if (c == 'n') { Serial.println("FAIL"); return T_FAIL; }
            if (c == 's') { Serial.println("skip"); return T_SKIP; }
        }
    }
    Serial.println("timeout - skipped");
    return T_SKIP;
}

static Result waitForButton(ButtonId b, const char* name) {
    Serial.printf("   Press %s within 10 s...", name);
    uint32_t start = millis();
    while (millis() - start < 10000) {
        tick();
        if (buttonDown(b)) {
            Serial.println(" detected  PASS");
            while (buttonDown(b)) tick();                  // wait for release
            buttonPressed(b);                              // clear the event
            return T_PASS;
        }
    }
    Serial.println(" not detected  FAIL");
    return T_FAIL;
}

// Spin one motor forward then backward and check its encoder follows.
static Result motorAndEncoderTest(uint8_t m) {
    const char* name = m == 0 ? "Motor A (J5)" : "Motor B (J6)";
    int sign[2] = { +1, -1 };
    long moved[2];

    for (int dir = 0; dir < 2; dir++) {
        long before = encoderCount(m);
        int s = 40 * sign[dir];
        Serial.printf("\n[%s] %s at 40%% for 1.5 s...\n", name, dir == 0 ? "forward" : "reverse");
        driveMotors(m == 0 ? s : 0, m == 1 ? s : 0);
        waitMs(1500);
        stopMotors(true);
        waitMs(300);
        moved[dir] = encoderCount(m) - before;
        Serial.printf("   encoder moved %ld counts\n", moved[dir]);
    }

    Result spun = ask("Did the motor spin forward, then backward (and only this one)?");
    if (spun != T_PASS) return spun;

    if (labs(moved[0]) < 20 && labs(moved[1]) < 20) {
        Serial.println(F("   Encoder did not count - check the encoder wires / 3V3 on the JST."));
        return T_FAIL;
    }
    if (moved[0] < 0 && moved[1] > 0) {
        Serial.printf("   Encoder counts the wrong way: set ENC_%c_INVERTED true in config.h\n",
                      m == 0 ? 'A' : 'B');
        return T_FAIL;
    }
    if (!(moved[0] > 0 && moved[1] < 0)) {
        Serial.println(F("   Encoder direction inconsistent - check channel A/B wiring."));
        return T_FAIL;
    }
    Serial.println(F("   Encoder follows the motor  PASS"));
    return T_PASS;
}

void runSelfTest() {
    resultCount = 0;
    stopAllModes();
    stopMotors(false);
    bool ledsWereAuto = ledsAuto();
    uint32_t savedRamp = motors.rampTime();
    motors.setRampTime(300);

    Serial.println(F("\n========== SELF TEST =========="));
    Serial.println(F("Answer with y (pass), n (fail) or s (skip). Wheels off the table!\n"));

    // ---- 1. MCU / USB - if you can read this, it works
    printBoardInfo();
    record("USB serial + ESP32-S2", T_PASS);

    // ---- 2. Power rails
    Serial.println(F("\n[POWER] With a meter, check 3V3 (controller J7 pin 10) and +5V (BOB J8 pin 2)."));
    record("3.3 V and 5 V rails", ask("3V3 = 3.2-3.4 V and +5V = 4.9-5.1 V?"));

    // ---- 3. LEDs
    Serial.println(F("\n[LEDS] Lighting red, green, blue, yellow in turn..."));
    for (uint8_t i = 0; i < LED_COUNT; i++) {
        ledsAllOff();
        ledSet((LedId)i, 255);
        Serial.printf("   %s\n", LED_NAMES[i]);
        waitMs(700);
    }
    ledsAllOff();
    record("4 LEDs", ask("Did all four light in order R, G, B, Y?"));

    // ---- 4. Buttons
    Serial.println(F("\n[BUTTONS]"));
    record("Button SW1 (IO8)",   waitForButton(BTN_SW1,  "SW1"));
    record("Button SW2 (IO13)",  waitForButton(BTN_SW2,  "SW2"));
    record("BOOT button (IO0)",  waitForButton(BTN_BOOT, "BOOT on the controller"));

    // ---- 5. OLED
    Serial.println(F("\n[OLED]"));
    if (displayOk()) {
        displayMessage("SELF TEST", "Can you read me?");
        record("OLED display", ask("Does the OLED say 'Can you read me?'"));
    } else {
        Serial.println(F("   OLED not found on I2C - running a bus scan:"));
        i2cScan();
        record("OLED display", T_FAIL);
    }

    // ---- 6. Servos
    Serial.println(F("\n[SERVOS] Plug servos into J8 and J9 (brown/black wire = GND)."));
    if (ask("Servos connected - sweep them now?") == T_PASS) {
        servoSweep(1);
        servoSweep(2);
        record("Servos J8 / J9", ask("Did both servos sweep and stop in the middle?"));
        servoOff(1); servoOff(2);
    } else record("Servos J8 / J9", T_SKIP);

    // ---- 7. Motors + encoders
    Serial.println(F("\n[MOTORS] Battery connected and encoder motors on J5 / J6?"));
    if (ask("Ready to spin motors?") == T_PASS) {
        motors.enable(true);
        encodersReset();
        record("Motor A + encoder", motorAndEncoderTest(0));
        record("Motor B + encoder", motorAndEncoderTest(1));

        Serial.println(F("\n[STBY] Both motors at 50%, then STBY drops after 1 s..."));
        driveMotors(50, 50);
        waitMs(1000);
        motors.enable(false);
        waitMs(1500);
        record("STBY stops motors", ask("Did both motors stop when STBY dropped?"));
        stopMotors(false);
    } else {
        record("Motor A + encoder", T_SKIP);
        record("Motor B + encoder", T_SKIP);
        record("STBY stops motors", T_SKIP);
    }

    // ---- 8. Line sensor
    Serial.println(F("\n[LINE] Connect the sensor bar to J15. Slide a black line under it."));
    if (ask("Watch the line sensor for 8 s?") == T_PASS) {
        uint8_t seen = 0;
        uint32_t start = millis(), last = 0;
        while (millis() - start < 8000) {
            tick();
            uint8_t m = lineRead();
            seen |= m;
            if (millis() - last > 250) { last = millis(); Serial.print("   "); linePrint(m); }
        }
        Serial.printf("   Sensors that saw the line: ");
        linePrint(seen);
        record("Line sensor J15", ask("Did each sensor react as the line passed?"));
    } else record("Line sensor J15", T_SKIP);

    // ---- 9. WiFi
    Serial.println(F("\n[WIFI]"));
    webPrintStatus();
    record("WiFi + web page", ask("Phone connects and the page loads?"));

    // ---- Summary
    int fails = 0;
    Serial.println(F("\n=========== RESULTS ==========="));
    for (uint8_t i = 0; i < resultCount; i++) {
        const char* s = results[i].r == T_PASS ? "PASS" : results[i].r == T_FAIL ? "FAIL" : "skip";
        if (results[i].r == T_FAIL) fails++;
        Serial.printf("  %-26s %s\n", results[i].name, s);
    }
    Serial.println(fails ? "\n  Some tests FAILED - see README 'Troubleshooting'."
                         : "\n  No failures. Board looks good!");
    Serial.println(F("==============================="));
    displayMessage(fails ? "Self test: FAILED" : "Self test: PASSED",
                   fails ? "see serial monitor" : "all good :)");

    motors.setRampTime(savedRamp);
    ledsSetAuto(ledsWereAuto);
}
