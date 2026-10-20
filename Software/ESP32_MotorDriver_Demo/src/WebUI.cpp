// =============================================================================
//  WebUI.cpp  -  WiFi + a tiny web server to control the robot from a phone.
//
//  WiFi behaviour
//    1. If a home/school network has been saved, join it (STA mode).
//       Browse to the IP shown on the OLED, or http://motordriver.local
//    2. Otherwise (or if joining fails) make our own hotspot (AP mode).
//       Phones get a "sign in to network" pop-up that opens the control page.
//    3. Save a network from the page's "WiFi setup" card, or with the serial
//       command "wifi set". It is kept in flash, even after re-uploading code.
//       Both normal (WPA2-Personal: one password) and WPA2-Enterprise
//       (username + password, e.g. eduroam with PEAP) networks are supported.
//    4. Hold SW1 while powering on (or type "wifi forget") to go back to hotspot.
//
//  Endpoints (handy for students writing their own apps / Python scripts):
//    GET  /                         the control page
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
//    GET  /api/wifi/forget          forget saved network and reboot
// =============================================================================
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include "App.h"
#include "Breakout.h"
#include "config.h"
#include "pins.h"
#include "WebPage.h"

static WebServer   server(80);
static DNSServer   dns;
static Preferences prefs;

static bool      driving = false;       // web page currently in control?
static uint32_t  lastDriveMs = 0;
static bool      staMode = false;
static String    apSsid;
static String    savedSsid, savedPass;  // network to join (flash or config.h)
static bool      savedEnterprise = false;
static String    savedUser, savedIdent; // enterprise username / outer identity
static uint8_t   savedEap = WPA2_AUTH_PEAP;
static String    wifiError;             // why joining failed, shown on the page
static uint32_t  rebootAt = 0;          // non-zero = reboot at this millis()

// ----------------------------------------------------------- saved network
static void loadCredentials() {
    prefs.begin("wifi", true);                       // read-only
    savedSsid = prefs.getString("ssid", WIFI_STA_SSID);
    savedPass = prefs.getString("pass", WIFI_STA_PASSWORD);
    savedEnterprise = prefs.getBool("ent", false);
    savedUser  = prefs.getString("user", "");
    savedIdent = prefs.getString("ident", "");
    savedEap   = prefs.getUChar("eap", WPA2_AUTH_PEAP);
    prefs.end();
}

void webSaveCredentials(const String& ssid, const String& pass) {
    prefs.begin("wifi", false);
    prefs.putString("ssid", ssid);
    prefs.putString("pass", pass);
    prefs.putBool("ent", false);
    prefs.putString("user", "");
    prefs.putString("ident", "");
    prefs.end();
}

// WPA2-Enterprise: identity is the "outer"/anonymous name (often the same as
// the username - leave it blank to use the username). eap = PEAP or TTLS.
void webSaveEnterprise(const String& ssid, const String& user, const String& pass,
                       const String& ident, bool ttls) {
    prefs.begin("wifi", false);
    prefs.putString("ssid", ssid);
    prefs.putString("pass", pass);
    prefs.putBool("ent", true);
    prefs.putString("user", user);
    prefs.putString("ident", ident);
    prefs.putUChar("eap", ttls ? WPA2_AUTH_TTLS : WPA2_AUTH_PEAP);
    prefs.end();
}

void webForgetCredentials() {
    prefs.begin("wifi", false);
    prefs.putString("ssid", "");                     // "" = use hotspot mode
    prefs.putString("pass", "");
    prefs.putBool("ent", false);
    prefs.putString("user", "");
    prefs.putString("ident", "");
    prefs.end();
}

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
static void handleRoot() { server.send_P(200, "text/html", INDEX_HTML); }

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

static void handleStatus() {
    char json[900];
    snprintf(json, sizeof(json),
        "{\"aNow\":%d,\"bNow\":%d,\"stby\":%d,\"mode\":\"%s\",\"demo\":%s,\"follow\":%s,"
        "\"ramp\":%lu,\"encA\":%ld,\"encB\":%ld,\"rateA\":%.0f,\"rateB\":%.0f,\"rateUnit\":\"%s\","
        "\"line\":%u,\"sw1\":%d,\"sw2\":%d,\"boot\":%d,\"servo1\":%d,\"servo2\":%d,"
        "\"ledsAuto\":%s,\"oled\":%s,\"uptime\":%lu,\"heap\":%u,\"temp\":%.1f,\"fw\":\"%s\","
        "\"wifiMode\":\"%s\",\"wifiSsid\":\"%s\",\"wifiIp\":\"%s\",\"wifiRssi\":%d,"
        "\"wifiSaved\":\"%s\",\"wifiSavedEnt\":%s,\"wifiError\":\"%s\"}",
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
        jsonSafe(savedSsid).c_str(), savedEnterprise ? "true" : "false",
        jsonSafe(wifiError).c_str());
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
    String ssid = server.arg("ssid");
    String pass = server.arg("pass");
    ssid.trim();
    if (ssid.isEmpty()) { server.send(400, "text/plain", "SSID is empty"); return; }

    if (server.arg("type") == "enterprise") {
        String user = server.arg("user");
        user.trim();
        if (user.isEmpty() || pass.isEmpty()) {
            server.send(400, "text/plain", "Enterprise WiFi needs a username and password");
            return;
        }
        String ident = server.arg("ident");
        ident.trim();
        webSaveEnterprise(ssid, user, pass, ident, server.arg("eap") == "ttls");
    } else {
        if (pass.length() > 0 && pass.length() < 8) {
            server.send(400, "text/plain", "WiFi passwords are at least 8 characters");
            return;
        }
        webSaveCredentials(ssid, pass);
    }
    Serial.printf("[wifi] saved \"%s\" - rebooting to join it\n", ssid.c_str());
    server.send(200, "text/plain", "Saved. Rebooting to join " + ssid + "...");
    webRebootSoon();
}

static void handleWifiForget() {
    webForgetCredentials();
    Serial.println(F("[wifi] saved network forgotten - rebooting into hotspot mode"));
    server.send(200, "text/plain", "Forgotten. Rebooting into hotspot mode...");
    webRebootSoon();
}

// --------------------------------------------------------- public helpers
void   webNotifyStopped() { driving = false; }
bool   webDriving()       { return driving; }
bool   webIsHotspot()     { return !staMode; }
int    webClients()       { return staMode ? 0 : WiFi.softAPgetStationNum(); }
String webSsid()          { return staMode ? WiFi.SSID() : apSsid; }
String webAddress()       { return staMode ? WiFi.localIP().toString() : WiFi.softAPIP().toString(); }
String webSavedSsid()     { return savedSsid; }

// ------------------------------------------------------------------- start
static bool joinNetwork() {
    Serial.printf("[wifi] joining \"%s\" ...\n", savedSsid.c_str());
    displayMessage("Joining WiFi...", savedSsid.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(MDNS_HOSTNAME);
    uint32_t timeout = WIFI_STA_TIMEOUT_MS;
    if (savedEnterprise) {
        // WPA2-Enterprise. No CA certificate is given, so the server's
        // certificate is NOT checked (common for hobby boards, but less secure).
        const String& ident = savedIdent.length() ? savedIdent : savedUser;
        Serial.printf("[wifi] enterprise login as \"%s\" (%s)\n", savedUser.c_str(),
                      savedEap == WPA2_AUTH_TTLS ? "TTLS" : "PEAP");
        WiFi.begin(savedSsid.c_str(), (wpa2_auth_method_t)savedEap,
                   ident.c_str(), savedUser.c_str(), savedPass.c_str());
        timeout = max<uint32_t>(timeout, 20000);    // enterprise logins are slower
    } else {
        WiFi.begin(savedSsid.c_str(), savedPass.c_str());
    }
    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < timeout) delay(100);
    if (WiFi.status() != WL_CONNECTED) {
        wifiError = "Could not join \"" + savedSsid + "\" - check the name/" +
                    (savedEnterprise ? "username/password" : "password");
        Serial.printf("[wifi] %s\n", wifiError.c_str());
        WiFi.disconnect(true);
        return false;
    }
    WiFi.setAutoReconnect(true);
    MDNS.begin(MDNS_HOSTNAME);
    MDNS.addService("http", "tcp", 80);
    return true;
}

static void startHotspot() {
    uint64_t mac = ESP.getEfuseMac();
    char suffix[5];
    snprintf(suffix, sizeof(suffix), "%02X%02X", (uint8_t)(mac >> 32), (uint8_t)(mac >> 40));
    apSsid = String(WIFI_AP_SSID_PREFIX) + suffix;
    WiFi.mode(WIFI_AP_STA);                          // AP + STA so we can scan
    WiFi.softAP(apSsid.c_str(), WIFI_AP_PASSWORD);
    dns.start(53, "*", WiFi.softAPIP());             // captive portal: every name -> us
}

void webBegin(bool forceHotspot) {
    loadCredentials();
    if (forceHotspot) {
        webForgetCredentials();
        savedSsid = "";
        Serial.println(F("[wifi] SW1 held at power-up - saved network forgotten"));
        displayMessage("WiFi reset", "hotspot mode");
        delay(1000);
    }

    staMode = savedSsid.length() > 0 && joinNetwork();
    if (!staMode) startHotspot();

    server.on("/",                handleRoot);
    server.on("/api/drive",       handleDrive);
    server.on("/api/stop",        handleStop);
    server.on("/api/demo",        handleDemo);
    server.on("/api/follow",      handleFollow);
    server.on("/api/ramp",        handleRamp);
    server.on("/api/servo",       handleServo);
    server.on("/api/led",         handleLed);
    server.on("/api/enc/reset",   handleEncReset);
    server.on("/api/status",      handleStatus);
    server.on("/api/wifi/scan",   handleWifiScan);
    server.on("/api/wifi/save",   HTTP_POST, handleWifiSave);
    server.on("/api/wifi/forget", handleWifiForget);
    server.onNotFound(handleNotFound);
    server.begin();
}

void webHandle() {
    if (!staMode) dns.processNextRequest();
    server.handleClient();

    // Dead-man: page stopped talking to us while motors were running
    if (driving && millis() - lastDriveMs > WEB_DEADMAN_MS) {
        driving = false;
        stopMotors(false);
        Serial.println(F("[web] no commands for a while - motors stopped (dead-man)"));
    }

    if (rebootAt && (int32_t)(millis() - rebootAt) > 0) ESP.restart();
}

void webPrintStatus() {
    if (staMode) {
        Serial.printf("  WiFi       : joined \"%s\" (%d dBm)%s\n", WiFi.SSID().c_str(), WiFi.RSSI(),
                      savedEnterprise ? "  [enterprise]" : "");
        Serial.printf("               browse to http://%s  or  http://%s.local\n",
                      webAddress().c_str(), MDNS_HOSTNAME);
    } else {
        Serial.printf("  WiFi       : hotspot \"%s\"  password \"%s\"\n",
                      apSsid.c_str(), WIFI_AP_PASSWORD);
        Serial.printf("               then browse to http://%s\n", webAddress().c_str());
        if (wifiError.length()) Serial.printf("               (%s)\n", wifiError.c_str());
        Serial.println(F("               save your own network with:  wifi set"));
    }
}

// Blocking scan for the serial "wifi scan" command
void webScanToSerial() {
    Serial.println(F("Scanning (about 3 s)..."));
    WiFi.scanDelete();
    int n = WiFi.scanNetworks();
    if (n <= 0) { Serial.println(F("  no networks found")); return; }
    for (int i = 0; i < n; i++) {
        wifi_auth_mode_t a = WiFi.encryptionType(i);
        Serial.printf("  %-32s %4d dBm %s\n", WiFi.SSID(i).c_str(), WiFi.RSSI(i),
                      a == WIFI_AUTH_OPEN ? "(open)" : a == WIFI_AUTH_WPA2_ENTERPRISE ? "(enterprise)" : "");
    }
    WiFi.scanDelete();
}
