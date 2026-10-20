// =============================================================================
//  SerialMenu.cpp  -  Type commands in the serial monitor.
//  (PlatformIO: click the plug icon in the status bar, then type + Enter)
// =============================================================================
#include <Arduino.h>
#include "App.h"
#include "Breakout.h"
#include "HeaderIO.h"
#include "pins.h"
#include "config.h"

void menuPrintHelp() {
    Serial.println(F(
        "\nCommands  (speeds are -100..100 %)\n"
        "  Motors\n"
        "    a <speed>          motor A                 e.g.  a 50\n"
        "    b <speed>          motor B                 e.g.  b -30\n"
        "    d <a> <b>          both motors             e.g.  d 60 60\n"
        "    stop | brake       stop everything (coast / brake)\n"
        "    stby <0|1>         driver standby on/off\n"
        "    ramp <ms>          ramp time 0->100 %  (0 = instant)\n"
        "    demo               start/stop the motor dance   (or SW1)\n"
        "  Breakout board\n"
        "    enc                encoder counts + speed (any key stops)\n"
        "    enc reset          zero the encoder counts\n"
        "    line               live line-sensor view (any key stops)\n"
        "    follow             start/stop line following   (or SW2)\n"
        "    servo <1|2> <deg>  move a servo (0..180)       e.g.  servo 1 90\n"
        "    servo <1|2> off    stop driving a servo\n"
        "    sweep <1|2>        sweep a servo end to end\n"
        "    led <r|g|b|y> <0-255>   set an LED     |  led auto  |  led off\n"
        "    buttons            show SW1 / SW2 / BOOT\n"
        "    oled               show the next OLED page   (or BOOT button)\n"
        "    i2c                scan the I2C bus (OLED = 0x3C)\n"
        "  WiFi\n"
        "    wifi               show WiFi status\n"
        "    wifi scan          list nearby networks (2.4 GHz only)\n"
        "    wifi set           save your network (home or enterprise - it asks)\n"
        "    wifi forget        forget it and go back to hotspot mode\n"
        "  Board\n"
        "    pins               controller header -> breakout pin map\n"
        "    blink <gpio>       raw pin test (reboot after)   |  blink all\n"
        "    read               raw level of every header pin\n"
        "    info               chip, memory, WiFi details\n"
        "    selftest           guided hardware test\n"
        "    help               this list\n"));
}

// Repeat 'fn' 5x a second until a key is pressed (keeps the robot running).
template <typename F> static void liveView(F fn) {
    while (Serial.available()) Serial.read();
    Serial.println(F("(press any key to stop)"));
    uint32_t last = 0;
    while (!Serial.available()) {
        webHandle(); motors.update(); encodersUpdate(); ledsUpdate(); displayUpdate();
        follower.update(); demo.update();
        if (millis() - last >= 200) { last = millis(); fn(); }
        delay(2);
    }
    while (Serial.available()) Serial.read();
}

// Read one full line (spaces allowed) - used for WiFi name / password.
static String readLine(const char* prompt, uint32_t timeoutMs = 60000) {
    while (Serial.available()) Serial.read();
    Serial.print(prompt);
    String line;
    uint32_t start = millis();
    while (millis() - start < timeoutMs) {
        webHandle(); motors.update();
        while (Serial.available()) {
            char ch = Serial.read();
            if (ch == '\r' || ch == '\n') {
                delay(5);                                    // swallow the LF of a CR+LF
                while (Serial.peek() == '\n') Serial.read();
                Serial.println();
                return line;
            } else {
                line += ch;
            }
        }
        delay(2);
    }
    Serial.println(F(" (timed out)"));
    return String();
}

static void wifiSetInteractive() {
    String ssid = readLine("WiFi network name (SSID): ");
    ssid.trim();
    if (ssid.isEmpty()) { Serial.println(F("Cancelled.")); return; }

    String type = readLine("Security: 1 = personal (one password), 2 = enterprise (username + password) [1]: ");
    type.trim();

    if (type == "2") {
        String user = readLine("Username: ");
        user.trim();
        String pass = readLine("Password: ");
        String eap  = readLine("Method: 1 = PEAP (most common), 2 = TTLS [1]: ");
        eap.trim();
        String ident = readLine("Anonymous identity (Enter = same as username): ");
        ident.trim();
        if (user.isEmpty() || pass.isEmpty()) {
            Serial.println(F("Enterprise WiFi needs a username and password - cancelled."));
            return;
        }
        webSaveEnterprise(ssid, user, pass, ident, eap == "2");
        Serial.println(F("Note: the server certificate is not checked (see README)."));
    } else {
        String pass = readLine("Password (blank for an open network): ");
        if (pass.length() > 0 && pass.length() < 8) {
            Serial.println(F("WiFi passwords are at least 8 characters - cancelled."));
            return;
        }
        webSaveCredentials(ssid, pass);
    }
    Serial.printf("Saved \"%s\". Rebooting to join it - the OLED will show the new address.\n",
                  ssid.c_str());
    webRebootSoon();
}

static bool parseLed(const char* s, LedId& out) {
    switch (s[0]) {
        case 'r': out = LED_RED;    return true;
        case 'g': out = LED_GREEN;  return true;
        case 'b': out = LED_BLUE;   return true;
        case 'y': out = LED_YELLOW; return true;
    }
    return false;
}

static void doCommand(char* line) {
    char* cmd = strtok(line, " ");
    if (!cmd) return;
    char* p1 = strtok(nullptr, " ");
    char* p2 = strtok(nullptr, " ");
    String c  = String(cmd); c.toLowerCase();
    String s1 = p1 ? String(p1) : String(); s1.toLowerCase();

    // ---- motors
    if      (c == "a" && p1)            { int b = motors.B.target(); stopAllModes(); driveMotors(atoi(p1), b); }
    else if (c == "b" && p1)            { int a = motors.A.target(); stopAllModes(); driveMotors(a, atoi(p1)); }
    else if (c == "d" && p1 && p2)      { stopAllModes(); driveMotors(atoi(p1), atoi(p2)); }
    else if (c == "stop")               { stopAllModes(); stopMotors(false); }
    else if (c == "brake")              { stopAllModes(); stopMotors(true); }
    else if (c == "stby" && p1)         { motors.enable(atoi(p1) != 0); }
    else if (c == "ramp" && p1)         { motors.setRampTime(constrain(atoi(p1), 0, 5000)); }
    else if (c == "demo")               { demo.toggle(); }

    // ---- breakout
    else if (c == "enc" && s1 == "reset") { encodersReset(); }
    else if (c == "enc") {
        liveView([] {
            Serial.printf("A: %8ld  %7.1f %s     B: %8ld  %7.1f %s\n",
                          encoderCount(0), encoderRate(0), encoderRateUnit(),
                          encoderCount(1), encoderRate(1), encoderRateUnit());
        });
    }
    else if (c == "line")               { liveView([] { linePrint(lineRead()); }); }
    else if (c == "follow")             { follower.toggle(); }
    else if (c == "servo" && p1 && p2) {
        if (String(p2) == "off") servoOff(atoi(p1));
        else                     servoWrite(atoi(p1), atoi(p2));
    }
    else if (c == "sweep" && p1)        { servoSweep(atoi(p1)); }
    else if (c == "led" && s1 == "auto") { ledsSetAuto(true); }
    else if (c == "led" && s1 == "off")  { ledsSetAuto(false); }
    else if (c == "led" && p1 && p2) {
        LedId id;
        if (parseLed(s1.c_str(), id)) ledSet(id, constrain(atoi(p2), 0, 255));
        else Serial.println(F("LED must be r, g, b or y"));
    }
    else if (c == "buttons") {
        liveView([] {
            Serial.printf("SW1 %s   SW2 %s   BOOT %s\n",
                          buttonDown(BTN_SW1) ? "DOWN" : "up  ",
                          buttonDown(BTN_SW2) ? "DOWN" : "up  ",
                          buttonDown(BTN_BOOT) ? "DOWN" : "up  ");
        });
    }
    else if (c == "oled")               { displayNextPage(); }
    else if (c == "i2c")                { i2cScan(); }

    // ---- board
    else if (c == "pins")               { headerPrintTable(); }
    else if (c == "blink" && p1) {
        if (s1 == "all") headerBlinkAll(2000);
        else {
            int g = atoi(p1);
            if (findHeaderPin(g)) { Serial.printf("Blinking IO%d for 5 s\n", g); headerBlink(g, 5000); }
            else Serial.println(F("Not a header pin - type 'pins' for the list"));
        }
    }
    else if (c == "read")               { headerReadDigital(); }
    else if (c == "info")               { printBoardInfo(); webPrintStatus(); }
    else if (c == "wifi" && s1 == "scan")   { webScanToSerial(); }
    else if (c == "wifi" && s1 == "set")    { wifiSetInteractive(); }
    else if (c == "wifi" && s1 == "forget") {
        webForgetCredentials();
        Serial.println(F("Forgotten. Rebooting into hotspot mode..."));
        webRebootSoon();
    }
    else if (c == "wifi")               { webPrintStatus(); }
    else if (c == "selftest")           { runSelfTest(); }
    else if (c == "help" || c == "?")   { menuPrintHelp(); }
    else { Serial.printf("Unknown command '%s' - type help\n", cmd); return; }

    Serial.printf("ok  (%s, A %d%%, B %d%%, driver %s)\n", modeName(),
                  motors.A.target(), motors.B.target(),
                  motors.enabled() ? "on" : "standby");
}

// Collect characters until Enter, then run the command.
void menuPoll() {
    static char    buf[64];
    static uint8_t len = 0;

    while (Serial.available()) {
        char ch = Serial.read();
        if (ch == '\r' || ch == '\n') {
            if (len) {
                buf[len] = 0;
                Serial.printf("> %s\n", buf);
                doCommand(buf);
                len = 0;
            }
        } else if (len < sizeof(buf) - 1) {
            buf[len++] = ch;
        }
    }
}
