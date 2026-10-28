// =============================================================================
//  SerialMenu.cpp  -  Type commands in the serial monitor.
//  (PlatformIO: click the plug icon in the status bar, then type + Enter)
// =============================================================================
#include <Arduino.h>
#include "Log.h"
#include "App.h"
#include "Breakout.h"
#include "HeaderIO.h"
#include "pins.h"
#include "config.h"
#include "Settings.h"
#include "SpeedControl.h"

void menuPrintHelp() {
    Log.println(F(
        "\r\nCommands  (speeds are -100..100 %)\r\n"
        "  Motors\r\n"
        "    a <speed>          motor A                 e.g.  a 50\r\n"
        "    b <speed>          motor B                 e.g.  b -30\r\n"
        "    d <a> <b>          both motors             e.g.  d 60 60\r\n"
        "    stop | brake       stop everything (coast / brake)\r\n"
        "    stby <0|1>         driver standby on/off\r\n"
        "    ramp <ms>          ramp time 0->100 %  (0 = instant)\r\n"
        "    demo               start/stop the motor dance   (or SW1)\r\n"
        "  Speed control (PID, uses the encoders)\r\n"
        "    pid                show settings          |  pid on  |  pid off\r\n"
        "    pid cal            measure top speed (wheels off the ground!)\r\n"
        "    pid kp|ki|kd|ks <value>   tune it, e.g. pid kp 0.8   (saved)\r\n"
        "    pid log            live target / measured / power (again = stop)\r\n"
        "  Breakout board\r\n"
        "    enc                encoder counts + speed (any key stops)\r\n"
        "    enc reset          zero the encoder counts\r\n"
        "    line               live line-sensor view (any key stops)\r\n"
        "    follow             start/stop line following   (or SW2)\r\n"
        "    servo <1|2> <deg>  move a servo (0..180)       e.g.  servo 1 90\r\n"
        "    servo <1|2> off    stop driving a servo\r\n"
        "    sweep <1|2>        sweep a servo end to end\r\n"
        "    led <r|g|b|y> <0-255>   set an LED     |  led auto  |  led off\r\n"
        "    buttons            show SW1 / SW2 / BOOT\r\n"
        "    oled               show the next OLED page   (or BOOT button)\r\n"
        "    i2c                scan the I2C bus (OLED = 0x3C)\r\n"
        "  WiFi\r\n"
        "    wifi               show WiFi status\r\n"
        "    wifi scan          list nearby networks (2.4 GHz only)\r\n"
        "    wifi set           add a network (home or enterprise - it asks)\r\n"
        "    wifi list          saved networks, in the order they are tried\r\n"
        "    wifi top <n>       try network n first     |  wifi remove <n>\r\n"
        "    wifi forget        forget ALL networks and go back to hotspot mode\r\n"
        "  Robot setup  (saved in flash)\r\n"
        "    name               show the robot's name (hotspot + http://<name>.local)\r\n"
        "    name <newname>     rename it, e.g.  name team3   (a-z 0-9 -, reboots)\r\n"
        "    name default       back to motordriver-xxxx\r\n"
        "    rev                show what is reversed\r\n"
        "    password <new>     set the robot password (web, hotspot, OTA; 8+ chars)\r\n"
        "    password default   back to the default (or hold SW2 at power-up)\r\n"
        "    rev <item>         reverse / un-reverse:  a  b  enca  encb  s1  s2\r\n"
        "  Board\r\n"
        "    pins               controller header -> breakout pin map\r\n"
        "    blink <gpio>       raw pin test (reboot after)   |  blink all\r\n"
        "    read               raw level of every header pin\r\n"
        "    info               chip, memory, WiFi details\r\n"
        "    selftest           guided hardware test\r\n"
        "    reboot             restart the board\r\n"
        "    help               this list\r\n"));
}

// Repeat 'fn' 5x a second until a key is pressed (keeps the robot running).
template <typename F> static void liveView(F fn) {
    while (Serial.available()) Serial.read();
    Log.println(F("(press any key to stop)"));
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
    Log.print(prompt);
    String line;
    uint32_t start = millis();
    while (millis() - start < timeoutMs) {
        webHandle(); motors.update();
        while (Serial.available()) {
            char ch = Serial.read();
            if (ch == '\r' || ch == '\n') {
                delay(5);                                    // swallow the LF of a CR+LF
                while (Serial.peek() == '\n') Serial.read();
                Log.println();
                return line;
            } else {
                line += ch;
            }
        }
        delay(2);
    }
    Log.println(F(" (timed out)"));
    return String();
}

static void wifiSetInteractive() {
    String ssid = readLine("WiFi network name (SSID): ");
    ssid.trim();
    if (ssid.isEmpty()) { Log.println(F("Cancelled.")); return; }

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
            Log.println(F("Enterprise WiFi needs a username and password - cancelled."));
            return;
        }
        String err = webAddNetwork(ssid, pass, true, user, ident, eap == "2");
        if (err.length()) { Log.printf("Not saved: %s\r\n", err.c_str()); return; }
        Log.println(F("Note: the server certificate is not checked (see README)."));
    } else {
        String pass = readLine("Password (blank for an open network): ");
        String err = webAddNetwork(ssid, pass);
        if (err.length()) { Log.printf("Not saved: %s\r\n", err.c_str()); return; }
    }
    if (webIsHotspot()) {
        Log.printf("Saved \"%s\". Restarting to connect - the OLED will show the new address.\r\n",
                      ssid.c_str());
        webRebootSoon();
    } else {
        Log.printf("Saved \"%s\". It will be tried at the next start (type 'reboot' to restart now).\r\n",
                      ssid.c_str());
    }
}

static void wifiList() {
    Log.printf("Saved networks (tried in this order, max %d):\r\n", WIFI_MAX_NETWORKS);
    if (!webNetworkCount()) Log.println(F("  none - add one with:  wifi set"));
    for (uint8_t i = 0; i < webNetworkCount(); i++)
        Log.printf("  %u  %s%s%s\r\n", i + 1, webNetworkSsid(i).c_str(),
                      webNetworkEnt(i) ? "  (enterprise)" : "",
                      (!webIsHotspot() && webSsid() == webNetworkSsid(i)) ? "  <- connected" : "");
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

static void doCommand(char* line, bool fromWeb) {
    char* cmd = strtok(line, " ");
    if (!cmd) return;
    char* p1 = strtok(nullptr, " ");
    char* p2 = strtok(nullptr, " ");
    String c  = String(cmd); c.toLowerCase();
    String s1 = p1 ? String(p1) : String(); s1.toLowerCase();

    // These wait for key presses on the USB serial monitor (or tie the robot
    // up for a long time), so they can't be run from the web terminal.
    if (fromWeb && ((c == "enc" && s1 != "reset") || c == "line" || c == "buttons" ||
                    (c == "wifi" && s1 == "set") || c == "selftest" || c == "blink")) {
        Log.printf("'%s' needs the USB serial monitor (it waits for key presses).\r\n", cmd);
        if (c == "enc" || c == "line" || c == "buttons")
            Log.println(F("The web pages already show these live."));
        return;
    }

    // ---- motors
    if      (c == "a" && p1)            { int b = motorTarget(1); stopAllModes(); driveMotors(atoi(p1), b); }
    else if (c == "b" && p1)            { int a = motorTarget(0); stopAllModes(); driveMotors(a, atoi(p1)); }
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
            Log.printf("A: %8ld  %7.1f %s     B: %8ld  %7.1f %s\r\n",
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
        else Log.println(F("LED must be r, g, b or y"));
    }
    else if (c == "buttons") {
        liveView([] {
            Log.printf("SW1 %s   SW2 %s   BOOT %s\r\n",
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
            if (findHeaderPin(g)) { Log.printf("Blinking IO%d for 5 s\r\n", g); headerBlink(g, 5000); }
            else Log.println(F("Not a header pin - type 'pins' for the list"));
        }
    }
    else if (c == "read")               { headerReadDigital(); }
    else if (c == "info")               { printBoardInfo(); webPrintStatus(); }
    else if (c == "wifi" && s1 == "scan")   { webScanToSerial(); }
    else if (c == "wifi" && s1 == "set")    { wifiSetInteractive(); }
    else if (c == "wifi" && s1 == "list")   { wifiList(); }
    else if (c == "wifi" && (s1 == "remove" || s1 == "top") && p2) {
        int n = atoi(p2) - 1;
        bool done = s1 == "remove" ? webRemoveNetwork(n) : webNetworkToTop(n);
        if (!done) Log.println(F("No such network - see 'wifi list'"));
        wifiList();
    }
    else if (c == "wifi" && s1 == "forget") {
        webForgetCredentials();
        Log.println(F("All networks forgotten. Rebooting into hotspot mode..."));
        webRebootSoon();
    }
    else if (c == "reboot")             { Log.println(F("Restarting...")); webRebootSoon(); }
    else if (c == "wifi")               { webPrintStatus(); }
    else if (c == "name" && p1) {
        String why;
        String n = (s1 == "default") ? String() : String(p1);
        if (!settingsSetHostname(n, &why)) { Log.printf("Not saved: %s\r\n", why.c_str()); return; }
        Log.printf("Robot name is now \"%s\". Rebooting...\r\n", settingsHostname().c_str());
        webRebootSoon();
    }
    else if (c == "name") {
        Log.printf("Robot name: %s%s\r\n  hotspot \"%s\" / http://%s.local\r\n",
                      settingsHostname().c_str(), settingsHostnameCustom() ? "" : " (default)",
                      settingsHostname().c_str(), settingsHostname().c_str());
    }
    else if (c == "password" && p1) {
        String why;
        String pw = (s1 == "default") ? String() : String(p1);
        if (!settingsSetPassword(pw, &why)) { Log.printf("Not saved: %s\r\n", why.c_str()); return; }
        Log.printf("Robot password %s. Restarting so the hotspot and OTA use it...\r\n",
                   pw.length() ? "changed" : "back to the default");
        webRebootSoon();
    }
    else if (c == "password") {
        Log.println(settingsPasswordCustom() ? F("Password: set by you. Change with: password <new>")
                                             : F("Password: still the default. Set your own: password <new>"));
    }
    else if (c == "rev" && p1) {
        int i = settingsRevFromName(s1);
        if (i < 0) { Log.println(F("rev what?  a  b  enca  encb  s1  s2")); return; }
        if (i == REV_MOTOR_A || i == REV_MOTOR_B) { stopAllModes(); stopMotors(false); }
        settingsSetReversed((RevItem)i, !settingsReversed((RevItem)i));
    }
    else if (c == "rev") {
        for (uint8_t i = 0; i < REV_COUNT; i++)
            Log.printf("  %-7s %s\r\n", REV_NAMES[i], settingsReversed((RevItem)i) ? "REVERSED" : "normal");
    }
    else if (c == "pid" && (s1 == "on" || s1 == "off")) { speedEnable(s1 == "on"); }
    else if (c == "pid" && s1 == "cal")  { speedCalibrate(); }
    else if (c == "pid" && s1 == "log")  { speedSetLogging(!speedLogging());
                                           Log.printf("PID logging %s\r\n", speedLogging() ? "on" : "off"); }
    else if (c == "pid" && p2 && (s1 == "kp" || s1 == "ki" || s1 == "kd" || s1 == "ks")) {
        SpeedGains& g = speedGains();
        float v = atof(p2);
        if (v < 0 || v > 100) { Log.println(F("Use a value from 0 to 100")); return; }
        (s1 == "kp" ? g.kp : s1 == "ki" ? g.ki : s1 == "kd" ? g.kd : g.ks) = v;
        speedSaveGains();
        speedPrint();
    }
    else if (c == "pid")                { speedPrint(); }
    else if (c == "selftest")           { runSelfTest(); }
    else if (c == "help" || c == "?")   { menuPrintHelp(); }
    else { Log.printf("Unknown command '%s' - type help\r\n", cmd); return; }

    Log.printf("ok  (%s, A %d%%, B %d%%, driver %s)\r\n", modeName(),
                  motorTarget(0), motorTarget(1),
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
                Log.printf("> %s\r\n", buf);
                doCommand(buf, false);
                len = 0;
            }
        } else if (len < sizeof(buf) - 1) {
            buf[len++] = ch;
        }
    }
}

// A command typed into a web page's Terminal (output appears there via Log)
void menuRunCommand(const String& text) {
    char buf[64];
    String t = text; t.trim();
    if (t.isEmpty()) return;
    strlcpy(buf, t.c_str(), sizeof(buf));
    Log.printf("[web] > %s\r\n", buf);
    doCommand(buf, true);
}
