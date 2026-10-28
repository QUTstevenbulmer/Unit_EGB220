// =============================================================================
//  WebUI.cpp  -  WiFi + a tiny web server to control the robot from a phone.
//
//  WiFi behaviour
//    1. Up to WIFI_MAX_NETWORKS networks can be saved (e.g. QUT, home, phone).
//       At power-up it scans and joins the first saved one that is in range.
//       Browse to the IP shown on the OLED, or http://<robot name>.local
//    2. Otherwise (or if joining fails) make our own hotspot (AP mode),
//       named after the robot (default motordriver-xxxx, see Settings.cpp).
//       Phones get a "sign in to network" pop-up that opens the control page.
//    3. Save a network from the page's "WiFi setup" card, or with the serial
//       command "wifi set". It is kept in flash, even after re-uploading code.
//       Both normal (WPA2-Personal: one password) and WPA2-Enterprise
//       (username + password, e.g. eduroam with PEAP) networks are supported.
//    4. Hold SW1 while powering on (or type "wifi forget") to go back to hotspot.
//
//  Endpoints (handy for students writing their own apps / Python scripts):
//    GET  /                         test & debug page (every device)
//    GET  /drive                    drive page: line sensor + joystick
//    GET  /api/log?from=N           web terminal text (X-Next header = new N)
//    POST /api/cmd                  cmd=<serial command>, e.g. cmd=d 30 30
//    GET  /api/drive?a=50&b=-50     motor targets in percent (-100..100)
//    GET  /api/stop[?brake=1]       stop everything (coast, or brake)
//    GET  /api/demo                 start/stop the motor demo
//    GET  /api/follow               start/stop line following
//    GET  /api/ramp?ms=600          ramp time
//    GET  /api/servo?n=1&deg=90     servo 1 or 2 to an angle (deg=-1 = off)
//    GET  /api/led?c=r&v=255        LED r/g/b/y brightness  (c=auto = status mode)
//    GET  /api/enc/reset            zero the encoder counts
//    GET  /api/status               JSON status of everything
//    GET  /api/wifi/scan            start / read a WiFi network scan
//    POST /api/wifi/save            ssid, pass [, type=enterprise, user, ident, eap=peap|ttls]
//    GET  /api/wifi/forget          forget ALL saved networks and reboot
//    GET  /api/wifi/list            saved networks (no passwords), in try order
//    GET  /api/wifi/remove?i=0      remove saved network i
//    GET  /api/wifi/top?i=2         move network i to the top (tried first)
//    GET  /api/wifi/restart         reboot and look for saved networks again
//    GET  /api/config               JSON: robot name + reverse settings
//    GET  /api/config?motorA=1      set reverse options (motorA, motorB, encA,
//                                   encB, servo1, servo2 = 0 or 1) - saved
//    POST /api/name                 name=<hostname>  ("" = default) - reboots
//    GET  /api/pid[?on=1&kp=..&ki=..&kd=..&ks=..&cal=1]   speed control (PID)
//    GET  /api/setup/spin?m=0       spin motor A (m=1: B) forward 1 s, returns encoder counts
//    POST /api/update               multipart upload of firmware.bin
//    GET/POST /login                log in (pw=<robot password>) -> session cookie
//    GET  /logout
//    POST /api/password             current=<old>, pw=<new> ("" = default) - reboots
//
//  Everything except /login needs the robot password (see config.h): log in
//  once per browser session. Python:  s = requests.Session();
//  s.post(url + "/login", data={"pw": "motors123"}); s.get(url + "/api/status")
//
//  Over-the-air updates also work from PlatformIO / Arduino IDE (ArduinoOTA,
//  port 3232, the robot password) - see README section "OTA updates".
// =============================================================================
#include <WiFi.h>
#include "Log.h"
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <Update.h>
#include <ArduinoOTA.h>
#include <esp_wpa2.h>
#include "App.h"
#include "Breakout.h"
#include "config.h"
#include "pins.h"
#include "Settings.h"
#include "SpeedControl.h"
#include "WebPage.h"
#include "DrivePage.h"
#include "TermJs.h"
#include "LoginPage.h"

static WebServer   server(80);
static DNSServer   dns;
static Preferences prefs;

static bool      driving = false;       // web page currently in control?
static uint32_t  lastDriveMs = 0;
static bool      staMode = false;
static String    apSsid;
// Saved networks - tried in this order at power-up (see joinSavedNetworks)
struct SavedNet {
    String  ssid, pass;
    bool    ent = false;          // WPA2-Enterprise (username + password)?
    String  user, ident;          // enterprise username / outer identity
    uint8_t eap = WPA2_AUTH_PEAP;
};
static SavedNet  nets[WIFI_MAX_NETWORKS];
static uint8_t   netCount = 0;
static int       joinedNet = -1;        // which saved network we're on (-1 = hotspot)
static String    wifiError;             // why joining failed, shown on the page
static uint32_t  rebootAt = 0;          // non-zero = reboot at this millis()
static bool      otaBusy = false;       // an update is being received
static String    otaError;              // why the web update failed

// ----------------------------------------------------------- saved networks
// Stored with Preferences in namespace "wifi":  n = count, then ssid0, pass0,
// ent0, user0, ident0, eap0, ssid1 ... for each network.
static String key(const char* base, uint8_t i) { return String(base) + i; }

static void saveNetworks() {
    prefs.begin("wifi", false);
    prefs.putUChar("n", netCount);
    for (uint8_t i = 0; i < netCount; i++) {
        prefs.putString(key("ssid", i).c_str(),  nets[i].ssid);
        prefs.putString(key("pass", i).c_str(),  nets[i].pass);
        prefs.putBool(key("ent", i).c_str(),     nets[i].ent);
        prefs.putString(key("user", i).c_str(),  nets[i].user);
        prefs.putString(key("ident", i).c_str(), nets[i].ident);
        prefs.putUChar(key("eap", i).c_str(),    nets[i].eap);
    }
    // Wipe removed entries and the old single-network keys, so no old
    // passwords are left lying around in flash
    const char* bases[] = { "ssid", "pass", "ent", "user", "ident", "eap" };
    for (uint8_t i = netCount; i < WIFI_MAX_NETWORKS; i++)
        for (const char* b : bases) if (prefs.isKey(key(b, i).c_str())) prefs.remove(key(b, i).c_str());
    for (const char* b : bases) if (prefs.isKey(b)) prefs.remove(b);
    prefs.end();
}

static void loadNetworks() {
    netCount = 0;
    prefs.begin("wifi", true);                       // read-only
    if (prefs.isKey("n")) {
        uint8_t n = min<uint8_t>(prefs.getUChar("n", 0), WIFI_MAX_NETWORKS);
        for (uint8_t i = 0; i < n; i++) {
            SavedNet& w = nets[netCount];
            w.ssid  = prefs.getString(key("ssid", i).c_str(), "");
            w.pass  = prefs.getString(key("pass", i).c_str(), "");
            w.ent   = prefs.getBool(key("ent", i).c_str(), false);
            w.user  = prefs.getString(key("user", i).c_str(), "");
            w.ident = prefs.getString(key("ident", i).c_str(), "");
            w.eap   = prefs.getUChar(key("eap", i).c_str(), WPA2_AUTH_PEAP);
            if (w.ssid.length()) netCount++;
        }
        prefs.end();
        return;
    }
    // First boot after updating from v1.4.x: bring the single saved network across
    SavedNet& w = nets[0];
    w.ssid  = prefs.getString("ssid", "");
    w.pass  = prefs.getString("pass", "");
    w.ent   = prefs.getBool("ent", false);
    w.user  = prefs.getString("user", "");
    w.ident = prefs.getString("ident", "");
    w.eap   = prefs.getUChar("eap", WPA2_AUTH_PEAP);
    prefs.end();
    if (w.ssid.length()) netCount = 1;
    else if (strlen(WIFI_STA_SSID)) {                // default network from config.h
        w = SavedNet();
        w.ssid = WIFI_STA_SSID; w.pass = WIFI_STA_PASSWORD;
        netCount = 1;
    }
    saveNetworks();
}

// Add a network (or update it if that name is already saved).
// Returns "" if OK, otherwise a message saying what's wrong.
String webAddNetwork(String ssid, const String& pass, bool ent,
                     String user, String ident, bool ttls) {
    ssid.trim(); user.trim(); ident.trim();
    if (ssid.isEmpty()) return "Network name is empty";
    if (ent && (user.isEmpty() || pass.isEmpty())) return "Enterprise WiFi needs a username and password";
    if (!ent && pass.length() > 0 && pass.length() < 8) return "WiFi passwords are at least 8 characters";
    int i = 0;
    while (i < netCount && nets[i].ssid != ssid) i++;
    if (i == netCount) {
        if (netCount >= WIFI_MAX_NETWORKS)
            return "Already " + String(WIFI_MAX_NETWORKS) + " networks saved - remove one first";
        netCount++;
    }
    SavedNet& w = nets[i];
    w.ssid = ssid; w.pass = pass; w.ent = ent;
    // Enterprise: identity is the "outer"/anonymous name (blank = use the username)
    w.user  = ent ? user : String();
    w.ident = ent ? ident : String();
    w.eap   = ttls ? WPA2_AUTH_TTLS : WPA2_AUTH_PEAP;
    saveNetworks();
    Log.printf("[wifi] saved \"%s\" (%u of %u)\r\n", ssid.c_str(), i + 1, WIFI_MAX_NETWORKS);
    return "";
}

bool webRemoveNetwork(uint8_t i) {
    if (i >= netCount) return false;
    Log.printf("[wifi] removed \"%s\"\r\n", nets[i].ssid.c_str());
    for (uint8_t j = i; j + 1 < netCount; j++) nets[j] = nets[j + 1];
    nets[--netCount] = SavedNet();
    if (joinedNet == i) joinedNet = -2;              // still connected, but no longer saved
    else if (joinedNet > i) joinedNet--;
    saveNetworks();
    return true;
}

// Move a network to the top of the list = tried first
bool webNetworkToTop(uint8_t i) {
    if (i >= netCount) return false;
    SavedNet w = nets[i];
    for (uint8_t j = i; j > 0; j--) nets[j] = nets[j - 1];
    nets[0] = w;
    if (joinedNet == i) joinedNet = 0;
    else if (joinedNet >= 0 && joinedNet < i) joinedNet++;
    saveNetworks();
    return true;
}

void webForgetCredentials() {
    for (auto& w : nets) w = SavedNet();
    netCount = 0;
    saveNetworks();
}

uint8_t webNetworkCount()           { return netCount; }
String  webNetworkSsid(uint8_t i)   { return i < netCount ? nets[i].ssid : String(); }
bool    webNetworkEnt(uint8_t i)    { return i < netCount && nets[i].ent; }

void webRebootSoon() {
    stopAllModes();
    stopMotors(false);
    rebootAt = millis() + 1500;                      // let the reply get sent first
}

// JSON strings can't contain raw quotes or backslashes
static String jsonSafe(String s) {
    s.replace("\\", "\\\\");
    s.replace("\"", "\\\"");
    return s;
}

// ------------------------------------------------------------------ helpers
static int argInt(const char* name, int fallback) {
    return server.hasArg(name) ? server.arg(name).toInt() : fallback;
}
static void ok() { server.send(200, "text/plain", "ok"); }

// ---------------------------------------------------------------- handlers
// ------------------------------------------------------------- log in
// One robot password (Settings.cpp). Logging in gives the browser a random
// session cookie; the robot remembers the last few. A restart forgets them
// all, and closing the browser drops the cookie - so it asks again.
static const uint8_t MAX_SESSIONS = 8;
static String   sessions[MAX_SESSIONS];
static uint8_t  nextSession = 0;

static String cookieToken() {
    String c = server.header("Cookie");
    int i = c.indexOf("sid=");
    if (i < 0) return String();
    int j = c.indexOf(';', i);
    return c.substring(i + 4, j < 0 ? c.length() : j);
}

static bool loggedIn() {
    if (!WEB_LOGIN_REQUIRED) return true;
    String t = cookieToken();
    if (t.length() != 16) return false;
    for (auto& s : sessions) if (s == t) return true;
    return false;
}

// Not logged in: send pages to the log-in page, refuse API calls.
static bool guard(bool isPage) {
    if (loggedIn()) return true;
    if (isPage) {
        server.sendHeader("Location", "/login?next=" + server.uri(), true);
        server.send(302, "text/plain", "");
    } else {
        server.send(401, "text/plain", "Log in first (POST /login with pw=...)");
    }
    return false;
}
static void page(const char* uri, void (*fn)()) { server.on(uri, [fn] { if (guard(true)) fn(); }); }
static void api(const char* uri, void (*fn)(), HTTPMethod m = HTTP_ANY) {
    server.on(uri, m, [fn] { if (guard(false)) fn(); });
}

static void sendLogin(const String& error) {
    String html = FPSTR(LOGIN_HTML);
    String next = server.arg("next");
    if (!next.startsWith("/") || next.startsWith("//")) next = "/";
    html.replace("%NAME%", settingsHostname());
    html.replace("%NEXT%", next);
    html.replace("%ERROR%", error);
    html.replace("%HINT%", settingsPasswordCustom() ? ""
        : "This robot still has the default password (" ROBOT_PASSWORD "). "
          "Set your own in Robot setup so other teams can't drive it.");
    server.send(error.length() ? 403 : 200, "text/html", html);
}

static void handleLoginPage() {
    if (loggedIn()) { server.sendHeader("Location", "/", true); server.send(302, "text/plain", ""); return; }
    sendLogin("");
}

static void handleLogin() {
    if (server.arg("pw") != settingsPassword()) {
        delay(500);                                  // slows down guessing
        Log.printf("[web] wrong password from %s\r\n", server.client().remoteIP().toString().c_str());
        sendLogin("Wrong password - try again.");
        return;
    }
    char tok[17];
    snprintf(tok, sizeof(tok), "%08lx%08lx", (unsigned long)esp_random(), (unsigned long)esp_random());
    sessions[nextSession] = tok;
    nextSession = (nextSession + 1) % MAX_SESSIONS;
    Log.printf("[web] logged in from %s\r\n", server.client().remoteIP().toString().c_str());
    String next = server.arg("next");
    if (!next.startsWith("/") || next.startsWith("//")) next = "/";
    server.sendHeader("Set-Cookie", String("sid=") + tok + "; Path=/; HttpOnly; SameSite=Strict");
    server.sendHeader("Location", next, true);
    server.send(302, "text/plain", "logged in");
}

static void handleLogout() {
    String t = cookieToken();
    for (auto& s : sessions) if (t.length() && s == t) s = "";
    server.sendHeader("Set-Cookie", "sid=; Path=/; Max-Age=0");
    server.sendHeader("Location", "/login", true);
    server.send(302, "text/plain", "");
}

// Change the robot password (needs the current one too). Restarts so the
// hotspot and OTA use it; everyone has to log in again.
static void handlePassword() {
    if (server.arg("current") != settingsPassword()) {
        delay(500);
        server.send(403, "text/plain", "Current password is wrong");
        return;
    }
    String why;
    if (!settingsSetPassword(server.arg("pw"), &why)) { server.send(400, "text/plain", why); return; }
    Log.println(F("[web] robot password changed - restarting"));
    server.send(200, "text/plain", settingsPasswordCustom()
        ? "Password changed. Restarting - log in again with the new password (also the hotspot password)."
        : "Back to the default password. Restarting...");
    webRebootSoon();
}

static void handleRoot()  { server.send_P(200, "text/html", INDEX_HTML); }   // test & debug page
static void handleDrivePage() { server.send_P(200, "text/html", DRIVE_HTML); }
static void handleTermJs() {
    server.sendHeader("Cache-Control", "max-age=300");
    server.send_P(200, "application/javascript", TERM_JS);
}

// Web terminal: new Log output since ?from=N. X-Next says where to carry on.
static void handleLog() {
    long from = server.hasArg("from") ? server.arg("from").toInt() : -1;
    String text = Log.readSince(from);
    server.sendHeader("X-Next", String(from));
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "text/plain", text);
}

// Web terminal: run a command as if typed in the serial monitor
static void handleCmd() {
    menuRunCommand(server.arg("cmd"));
    ok();
}

// Any unknown URL: in hotspot mode send the phone to the control page.
// This is what makes the "sign in to network" pop-up appear.
static void handleNotFound() {
    if (!staMode) {
        server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/", true);
        server.send(302, "text/plain", "");
    } else {
        server.send(404, "text/plain", "not found");
    }
}

static void handleDrive() {
    if (!driving) { demo.stop(); follower.stop(); }
    driveMotors(argInt("a", 0), argInt("b", 0));
    driving     = true;
    lastDriveMs = millis();
    ok();
}

static void handleStop() {
    stopAllModes();
    stopMotors(argInt("brake", 0) == 1);
    ok();
}

static void handleDemo()   { driving = false; demo.toggle();     ok(); }
static void handleFollow() { driving = false; follower.toggle(); ok(); }

static void handleRamp() {
    motors.setRampTime(constrain(argInt("ms", MOTOR_RAMP_MS), 0, 5000));
    ok();
}

static void handleServo() {
    int n = argInt("n", 1), deg = argInt("deg", 90);
    if (deg < 0) servoOff(n); else servoWrite(n, deg);
    ok();
}

static void handleLed() {
    String c = server.arg("c");
    int v = constrain(argInt("v", 255), 0, 255);
    if      (c == "auto") ledsSetAuto(true);
    else if (c == "off")  ledsSetAuto(false);
    else if (c == "r") ledSet(LED_RED, v);
    else if (c == "g") ledSet(LED_GREEN, v);
    else if (c == "b") ledSet(LED_BLUE, v);
    else if (c == "y") ledSet(LED_YELLOW, v);
    ok();
}

static void handleEncReset() { encodersReset(); ok(); }

// Robot setup: reverse options (apply immediately) and name (needs a reboot)
static String configJson() {
    String j = "{\"name\":\"" + settingsHostname() + "\",\"nameCustom\":" +
               (settingsHostnameCustom() ? "true" : "false");
    for (uint8_t i = 0; i < REV_COUNT; i++)
        j += String(",\"") + REV_NAMES[i] + "\":" + (settingsReversed((RevItem)i) ? "true" : "false");
    return j + "}";
}

static void handleConfig() {
    bool motorChanged = false;
    for (uint8_t i = 0; i < REV_COUNT; i++) {
        if (!server.hasArg(REV_NAMES[i])) continue;
        bool on = server.arg(REV_NAMES[i]) == "1" || server.arg(REV_NAMES[i]) == "true";
        if (on != settingsReversed((RevItem)i)) {
            if (i == REV_MOTOR_A || i == REV_MOTOR_B) motorChanged = true;
            settingsSetReversed((RevItem)i, on);
        }
    }
    if (motorChanged) { stopAllModes(); stopMotors(false); }   // don't flip while moving
    server.send(200, "application/json", configJson());
}

// Speed control (PID): GET /api/pid  [?on=0|1] [&kp=..&ki=..&kd=..&ks=..] [&cal=1]
static void handlePid() {
    String msg;
    if (server.hasArg("cal")) msg = speedCalibrate() ? "Calibrated." : "Calibration failed - see the terminal.";
    if (server.hasArg("on") && !speedEnable(server.arg("on") == "1"))
        msg = "Measure top speed first (Calibrate).";
    SpeedGains& g = speedGains();
    bool changed = false;
    auto setGain = [&](const char* n, float& v) {
        if (!server.hasArg(n)) return;
        float x = server.arg(n).toFloat();
        if (x >= 0 && x <= 100) { v = x; changed = true; }
    };
    setGain("kp", g.kp); setGain("ki", g.ki); setGain("kd", g.kd); setGain("ks", g.ks);
    if (changed) speedSaveGains();
    char j[220];
    snprintf(j, sizeof(j), "{\"on\":%s,\"max\":%.0f,\"kp\":%.3f,\"ki\":%.3f,\"kd\":%.3f,\"ks\":%.3f,\"msg\":\"%s\"}",
             speedEnabled() ? "true" : "false", speedMaxCps(), g.kp, g.ki, g.kd, g.ks, msg.c_str());
    server.send(200, "application/json", j);
}

// Setup check: spin ONE motor "forward" at 40 % for 1 s (raw power, no PID)
// and report how far its encoder moved. The page asks which way it turned.
static void handleSetupSpin() {
    uint8_t m = argInt("m", 0) ? 1 : 0;
    stopAllModes();
    stopMotors(false);
    bool pid = speedEnabled();
    speedEnable(false, false);
    motors.enable(true);
    long before = encoderCount(m);
    (m == 0 ? motors.A : motors.B).setOutput(40);
    delay(1000);                                     // encoders count by interrupt
    stopMotors(true);
    delay(200);
    long moved = encoderCount(m) - before;
    if (pid) speedEnable(true, false);
    Log.printf("[setup] motor %c forward 1 s -> encoder %+ld counts\r\n", m ? 'B' : 'A', moved);
    server.send(200, "application/json", String("{\"counts\":") + moved + "}");
}

static void handleName() {
    String why;
    String name = server.arg("name");
    if (!settingsSetHostname(name, &why)) { server.send(400, "text/plain", why); return; }
    String now = settingsHostname();
    Log.printf("[cfg] robot name set to \"%s\" - rebooting\r\n", now.c_str());
    server.send(200, "text/plain", "Saved. Rebooting as \"" + now + "\"...");
    webRebootSoon();
}

static void handleStatus() {
    char json[1200];
    snprintf(json, sizeof(json),
        "{\"aNow\":%d,\"bNow\":%d,\"stby\":%d,\"mode\":\"%s\",\"demo\":%s,\"follow\":%s,"
        "\"ramp\":%lu,\"encA\":%ld,\"encB\":%ld,\"rateA\":%.0f,\"rateB\":%.0f,\"rateUnit\":\"%s\","
        "\"line\":%u,\"sw1\":%d,\"sw2\":%d,\"boot\":%d,\"servo1\":%d,\"servo2\":%d,"
        "\"ledsAuto\":%s,\"oled\":%s,\"uptime\":%lu,\"heap\":%u,\"temp\":%.1f,\"fw\":\"%s\","
        "\"wifiMode\":\"%s\",\"wifiSsid\":\"%s\",\"wifiIp\":\"%s\",\"wifiRssi\":%d,"
        "\"wifiSaved\":%u,\"wifiSavedEnt\":%s,\"wifiError\":\"%s\",\"name\":\"%s\","
        "\"pwCustom\":%s,\"pid\":%s,\"pidCal\":%s,\"tgtA\":%d,\"tgtB\":%d,\"measA\":%.0f,\"measB\":%.0f}",
        (int)motors.A.speed(), (int)motors.B.speed(), motors.enabled() ? 1 : 0, modeName(),
        demo.running() ? "true" : "false", follower.running() ? "true" : "false",
        (unsigned long)motors.rampTime(), encoderCount(0), encoderCount(1),
        encoderRate(0), encoderRate(1), encoderRateUnit(),
        lineRead(), buttonDown(BTN_SW1), buttonDown(BTN_SW2), buttonDown(BTN_BOOT),
        servoAngle(1), servoAngle(2), ledsAuto() ? "true" : "false",
        displayOk() ? "true" : "false", (unsigned long)(millis() / 1000),
        ESP.getFreeHeap(), temperatureRead(), FW_VERSION,
        staMode ? "STA" : "AP", jsonSafe(webSsid()).c_str(), webAddress().c_str(),
        staMode ? (int)WiFi.RSSI() : 0,
        netCount, (joinedNet >= 0 && nets[joinedNet].ent) ? "true" : "false",
        jsonSafe(wifiError).c_str(), settingsHostname().c_str(),
        settingsPasswordCustom() ? "true" : "false",
        speedEnabled() ? "true" : "false", speedMaxCps() > 0 ? "true" : "false",
        motorTarget(0), motorTarget(1), speedMeasured(0), speedMeasured(1));
    server.send(200, "application/json", json);
}

// Network scan runs in the background so the robot keeps running.
// The page calls this repeatedly until the list arrives.
static void handleWifiScan() {
    int n = WiFi.scanComplete();
    if (n == WIFI_SCAN_FAILED) {                     // not started yet
        WiFi.scanNetworks(true);
        server.send(200, "application/json", "{\"scanning\":true}");
        return;
    }
    if (n == WIFI_SCAN_RUNNING) {
        server.send(200, "application/json", "{\"scanning\":true}");
        return;
    }
    String json = "{\"scanning\":false,\"networks\":[";
    for (int i = 0; i < n; i++) {
        if (WiFi.SSID(i).isEmpty()) continue;        // hidden networks
        if (json.endsWith("}")) json += ",";
        json += "{\"ssid\":\"" + jsonSafe(WiFi.SSID(i)) + "\",\"rssi\":" + WiFi.RSSI(i) +
                ",\"open\":" + (WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "true" : "false") +
                ",\"ent\":"  + (WiFi.encryptionType(i) == WIFI_AUTH_WPA2_ENTERPRISE ? "true" : "false") + "}";
    }
    json += "]}";
    WiFi.scanDelete();                               // next call starts a new scan
    server.send(200, "application/json", json);
}

static void handleWifiSave() {
    String err = webAddNetwork(server.arg("ssid"), server.arg("pass"),
                               server.arg("type") == "enterprise", server.arg("user"),
                               server.arg("ident"), server.arg("eap") == "ttls");
    if (err.length()) { server.send(400, "text/plain", err); return; }
    String ssid = server.arg("ssid"); ssid.trim();
    if (!staMode) {
        // In hotspot mode there's nothing to lose - restart and try it now
        server.send(200, "text/plain", "Saved \"" + ssid + "\". Restarting to connect...");
        webRebootSoon();
    } else {
        server.send(200, "text/plain", "Saved \"" + ssid + "\". It will be tried next time the robot starts.");
    }
}

// List of saved networks (passwords are never sent back)
static void handleWifiList() {
    String j = "{\"max\":" + String(WIFI_MAX_NETWORKS) + ",\"joined\":" + String(joinedNet) + ",\"networks\":[";
    for (uint8_t i = 0; i < netCount; i++) {
        if (i) j += ",";
        j += "{\"ssid\":\"" + jsonSafe(nets[i].ssid) + "\",\"ent\":" + (nets[i].ent ? "true" : "false") + "}";
    }
    server.send(200, "application/json", j + "]}");
}

static void handleWifiRemove() { webRemoveNetwork(argInt("i", 255)); handleWifiList(); }
static void handleWifiTop()    { webNetworkToTop(argInt("i", 255));  handleWifiList(); }

static void handleWifiRestart() {
    server.send(200, "text/plain", "Restarting to look for saved networks...");
    webRebootSoon();
}

static void handleWifiForget() {
    webForgetCredentials();
    Log.println(F("[wifi] all saved networks forgotten - rebooting into hotspot mode"));
    server.send(200, "text/plain", "All networks forgotten. Restarting in hotspot mode...");
    webRebootSoon();
}

// ------------------------------------------------------------ OTA update
// Everything stops before new code is written - a robot shouldn't drive
// around while it is being reprogrammed.
static void otaStarting(const char* how) {
    otaBusy = true;
    stopAllModes();
    stopMotors(false);
    motors.enable(false);
    servoOff(1); servoOff(2);
    Log.printf("[ota] update started (%s) - motors off\r\n", how);
    displayMessage("Updating firmware", "don't power off!");
}

static void otaProgress(uint32_t done, uint32_t total) {
    static uint32_t lastMs = 0;
    if (millis() - lastMs < 500) return;
    lastMs = millis();
    char line[24];
    if (total) snprintf(line, sizeof(line), "%u %%", (unsigned)(done * 100ULL / total));
    else       snprintf(line, sizeof(line), "%u kB", (unsigned)(done / 1024));
    Log.printf("[ota] %s\r\n", line);
    displayMessage("Updating firmware", line);
}

// Web page upload: called repeatedly as chunks of the .bin file arrive
static void handleUpdateUpload() {
    HTTPUpload& up = server.upload();
    if (up.status == UPLOAD_FILE_START) {
        otaError = "";
        if (!loggedIn())                        { otaError = "Log in first"; return; }
        if (!up.filename.endsWith(".bin"))      { otaError = "Pick a .bin firmware file"; return; }
        otaStarting("web page");
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) otaError = Update.errorString();
    } else if (up.status == UPLOAD_FILE_WRITE) {
        if (otaError.length()) return;
        if (Update.write(up.buf, up.currentSize) != up.currentSize) otaError = Update.errorString();
        otaProgress(up.totalSize, 0);
    } else if (up.status == UPLOAD_FILE_END) {
        if (otaError.length()) return;
        if (!Update.end(true)) otaError = Update.errorString();      // true = size from upload
    } else if (up.status == UPLOAD_FILE_ABORTED) {
        Update.abort();
        otaError = "Upload interrupted";
    }
}

// Called once the whole upload has arrived
static void handleUpdateDone() {
    if (otaError.length() || !Update.isFinished()) {
        if (otaError.isEmpty()) otaError = "No firmware received";
        if (Update.isRunning()) Update.abort();
        Log.printf("[ota] failed: %s\r\n", otaError.c_str());
        if (otaBusy) displayMessage("Update FAILED", otaError.c_str());
        otaBusy = false;
        server.send(400, "text/plain", "Update failed: " + otaError);
        return;
    }
    Log.println(F("[ota] success - rebooting into the new firmware"));
    displayMessage("Update OK", "rebooting...");
    server.send(200, "text/plain", "Update OK. The robot is restarting with the new firmware...");
    otaBusy  = false;
    rebootAt = millis() + 1500;
}

// PlatformIO / Arduino IDE network upload (espota protocol)
static void otaBegin() {
    ArduinoOTA.setHostname(settingsHostname().c_str());
    ArduinoOTA.setPassword(settingsPassword().c_str());   // the robot password
    ArduinoOTA.onStart([] { otaStarting("network upload"); });
    ArduinoOTA.onProgress([](unsigned int done, unsigned int total) { otaProgress(done, total); });
    ArduinoOTA.onEnd([] { displayMessage("Update OK", "rebooting..."); });
    ArduinoOTA.onError([](ota_error_t e) {
        otaBusy = false;
        const char* why = e == OTA_AUTH_ERROR ? "wrong password" : e == OTA_BEGIN_ERROR ? "no space"
                        : e == OTA_CONNECT_ERROR ? "connect failed" : e == OTA_RECEIVE_ERROR ? "receive failed"
                        : "end failed";
        Log.printf("[ota] failed: %s\r\n", why);
        displayMessage("Update FAILED", why);
    });
    ArduinoOTA.begin();                              // also starts mDNS with our name
}

bool webUpdating() { return otaBusy; }

// --------------------------------------------------------- public helpers
void   webNotifyStopped() { driving = false; }
bool   webDriving()       { return driving; }
bool   webIsHotspot()     { return !staMode; }
int    webClients()       { return staMode ? 0 : WiFi.softAPgetStationNum(); }
String webSsid()          { return staMode ? WiFi.SSID() : apSsid; }
String webAddress()       { return staMode ? WiFi.localIP().toString() : WiFi.softAPIP().toString(); }

// ------------------------------------------------------------------- start
static bool joinNetwork(const SavedNet& w) {
    Log.printf("[wifi] trying \"%s\" ...\r\n", w.ssid.c_str());
    displayMessage("Joining WiFi...", w.ssid.c_str());
    uint32_t timeout = WIFI_STA_TIMEOUT_MS;
    if (w.ent) {
        // WPA2-Enterprise. No CA certificate is given, so the server's
        // certificate is NOT checked (common for hobby boards, but less secure).
        const String& ident = w.ident.length() ? w.ident : w.user;
        Log.printf("[wifi] enterprise login as \"%s\" (%s)\r\n", w.user.c_str(),
                      w.eap == WPA2_AUTH_TTLS ? "TTLS" : "PEAP");
        WiFi.begin(w.ssid.c_str(), (wpa2_auth_method_t)w.eap,
                   ident.c_str(), w.user.c_str(), w.pass.c_str());
        timeout = max<uint32_t>(timeout, 20000);    // enterprise logins are slower
    } else {
        esp_wifi_sta_wpa2_ent_disable();             // in case an enterprise one was tried first
        WiFi.begin(w.ssid.c_str(), w.pass.c_str());
    }
    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < timeout) delay(100);
    if (WiFi.status() == WL_CONNECTED) return true;
    Log.printf("[wifi] could not join \"%s\"\r\n", w.ssid.c_str());
    WiFi.disconnect(true);
    delay(200);
    return false;
}

// Scan, then try each saved network that is in range, in list order.
static bool joinSavedNetworks() {
    WiFi.setHostname(settingsHostname().c_str());   // name shown in the router's list
    WiFi.mode(WIFI_STA);
    displayMessage("Looking for WiFi", "");
    Log.printf("[wifi] %u saved network(s) - scanning...\r\n", netCount);
    int found = WiFi.scanNetworks();
    bool inRange[WIFI_MAX_NETWORKS] = {};
    for (int k = 0; k < found; k++)
        for (uint8_t i = 0; i < netCount; i++)
            if (WiFi.SSID(k) == nets[i].ssid) inRange[i] = true;
    WiFi.scanDelete();
    // If the scan failed, try everything. Otherwise only networks we can see
    // (saves 10-20 s for each one that isn't here).
    bool tryAll = found <= 0;
    String tried, missing;
    for (uint8_t i = 0; i < netCount; i++) {
        if (!tryAll && !inRange[i]) { missing += (missing.length() ? ", " : "") + nets[i].ssid; continue; }
        tried += (tried.length() ? ", " : "") + nets[i].ssid;
        if (joinNetwork(nets[i])) {
            joinedNet = i;
            WiFi.setAutoReconnect(true);
            return true;
        }
    }
    if (tried.length())
        wifiError = "Could not join " + tried + " - check the password" +
                    (missing.length() ? ". Not in range: " + missing : String());
    else
        wifiError = "None of the saved networks are in range (" + missing + ")";
    Log.printf("[wifi] %s\r\n", wifiError.c_str());
    return false;
}

static void startHotspot() {
    apSsid = settingsHostname();                     // hotspot is named after the robot
    WiFi.setHostname(apSsid.c_str());
    WiFi.mode(WIFI_AP_STA);                          // AP + STA so we can scan
    WiFi.softAP(apSsid.c_str(), settingsPassword().c_str());   // the robot password
    dns.start(53, "*", WiFi.softAPIP());             // captive portal: every name -> us
}

void webBegin(bool forceHotspot) {
    loadNetworks();
    if (forceHotspot) {
        webForgetCredentials();
        Log.println(F("[wifi] SW1 held at power-up - all saved networks forgotten"));
        displayMessage("WiFi reset", "hotspot mode");
        delay(1000);
    }

    staMode = netCount > 0 && joinSavedNetworks();
    if (!staMode) startHotspot();

    // http://<name>.local  (works on iPhone/Mac/Windows/Linux; some Android
    // phones don't support .local names - use the IP address there)
    if (MDNS.begin(settingsHostname().c_str())) MDNS.addService("http", "tcp", 80);
    otaBegin();                                      // adds the Arduino OTA service
    Log.printf("[wifi] robot name: %s  ->  http://%s.local\r\n",
                  settingsHostname().c_str(), settingsHostname().c_str());

    // Log-in pages are open to everyone; everything else needs the password
    const char* keepHeaders[] = { "Cookie" };
    server.collectHeaders(keepHeaders, 1);
    server.on("/login",  HTTP_GET,  handleLoginPage);
    server.on("/login",  HTTP_POST, handleLogin);
    server.on("/logout", handleLogout);
    api("/api/password", handlePassword, HTTP_POST);
    page("/",              handleRoot);
    page("/drive",         handleDrivePage);
    api("/term.js", handleTermJs);
    api("/api/log", handleLog);
    api("/api/cmd", handleCmd, HTTP_POST);
    api("/api/drive", handleDrive);
    api("/api/stop", handleStop);
    api("/api/demo", handleDemo);
    api("/api/follow", handleFollow);
    api("/api/ramp", handleRamp);
    api("/api/servo", handleServo);
    api("/api/led", handleLed);
    api("/api/enc/reset", handleEncReset);
    api("/api/status", handleStatus);
    api("/api/wifi/scan", handleWifiScan);
    api("/api/wifi/save", handleWifiSave, HTTP_POST);
    api("/api/wifi/forget", handleWifiForget);
    api("/api/wifi/list", handleWifiList);
    api("/api/wifi/remove", handleWifiRemove);
    api("/api/wifi/top", handleWifiTop);
    api("/api/wifi/restart", handleWifiRestart);
    api("/api/config", handleConfig);
    api("/api/name", handleName, HTTP_POST);
    api("/api/pid", handlePid);
    api("/api/setup/spin", handleSetupSpin);
    server.on("/api/update", HTTP_POST, [] { if (guard(false)) handleUpdateDone(); }, handleUpdateUpload);
    server.onNotFound(handleNotFound);
    server.begin();
}

void webHandle() {
    if (!staMode) dns.processNextRequest();
    server.handleClient();
    ArduinoOTA.handle();
    if (otaBusy) return;                             // no dead-man etc. mid-update

#if WIFI_LOST_RESTART_S > 0
    // Joined network has gone (robot carried out of range, router off...).
    // Once it has been gone a while and the robot is idle, restart: that
    // re-scans for the other saved networks, or makes the hotspot.
    static uint32_t lostSince = 0;
    if (staMode && WiFi.status() != WL_CONNECTED) {
        if (!lostSince) { lostSince = millis(); Log.println(F("[wifi] connection lost - retrying")); }
        if (millis() - lostSince > WIFI_LOST_RESTART_S * 1000UL && strcmp(modeName(), "Idle") == 0 && !rebootAt) {
            Log.println(F("[wifi] network still gone - restarting to look for others"));
            displayMessage("WiFi lost", "restarting...");
            webRebootSoon();
        }
    } else lostSince = 0;
#endif

    // Dead-man: page stopped talking to us while motors were running
    // (only matters - and only reported - if the motors were actually told to move)
    if (driving && millis() - lastDriveMs > WEB_DEADMAN_MS) {
        driving = false;
        if (motorTarget(0) != 0 || motorTarget(1) != 0) {
            stopMotors(false);
            Log.println(F("[web] lost contact with the page - motors stopped (dead-man)"));
        }
    }

    if (rebootAt && (int32_t)(millis() - rebootAt) > 0) ESP.restart();
}

void webPrintStatus() {
    if (staMode) {
        Log.printf("  WiFi       : joined \"%s\" (%d dBm)%s\r\n", WiFi.SSID().c_str(), WiFi.RSSI(),
                      (joinedNet >= 0 && nets[joinedNet].ent) ? "  [enterprise]" : "");
        Log.printf("               browse to http://%s  or  http://%s.local\r\n",
                      webAddress().c_str(), settingsHostname().c_str());
    } else {
        Log.printf("  WiFi       : hotspot \"%s\"  password %s\r\n", apSsid.c_str(),
                   settingsPasswordCustom() ? "= your robot password" : "\"" ROBOT_PASSWORD "\" (default)");
        Log.printf("               then browse to http://%s  or  http://%s.local\r\n",
                      webAddress().c_str(), settingsHostname().c_str());
        if (wifiError.length()) Log.printf("               (%s)\r\n", wifiError.c_str());
        Log.printf("               %u network(s) saved - add one with:  wifi set\r\n", netCount);
    }
}

// Blocking scan for the serial "wifi scan" command
void webScanToSerial() {
    Log.println(F("Scanning (about 3 s)..."));
    WiFi.scanDelete();
    int n = WiFi.scanNetworks();
    if (n <= 0) { Log.println(F("  no networks found")); return; }
    for (int i = 0; i < n; i++) {
        wifi_auth_mode_t a = WiFi.encryptionType(i);
        Log.printf("  %-32s %4d dBm %s\r\n", WiFi.SSID(i).c_str(), WiFi.RSSI(i),
                      a == WIFI_AUTH_OPEN ? "(open)" : a == WIFI_AUTH_WPA2_ENTERPRISE ? "(enterprise)" : "");
    }
    WiFi.scanDelete();
}
