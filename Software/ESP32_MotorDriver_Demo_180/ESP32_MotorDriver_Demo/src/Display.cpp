// =============================================================================
//  Display.cpp  -  0.91" 128x32 OLED (SSD1306) on the breakout board.
//
//  Press the BOOT button to flip between pages:
//      0  WiFi       network name, password (hotspot), address, mode
//      1  Motors     speed % and encoder count / speed for both motors
//      2  Line       live picture of the 8 line sensors
//      3  Inputs     buttons, servos, chip temperature
// =============================================================================
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Breakout.h"
#include "App.h"
#include "config.h"
#include "Settings.h"

static Adafruit_SSD1306 oled(128, 32, &Wire, -1);
static bool    ok = false;
static uint8_t page = 0;
static const uint8_t PAGE_COUNT = 4;
static uint32_t messageUntil = 0;

bool displayBegin() {
    // The Adafruit library's begin() doesn't check the display is really
    // there (it only reserves memory), so first see if anything answers on
    // the I2C bus at the OLED's address.
    Wire.beginTransmission(OLED_I2C_ADDRESS);
    if (Wire.endTransmission() != 0) { ok = false; return false; }
    ok = oled.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS);
    if (!ok) return false;
    oled.setTextColor(SSD1306_WHITE);
    displayMessage("ESP32-S2 Motor Driver", "fw " FW_VERSION);
    return true;
}

bool displayOk() { return ok; }

void displayNextPage() {
    page = (page + 1) % PAGE_COUNT;
    messageUntil = 0;
}

// Show two lines of text for 1.5 s (overrides the normal pages)
void displayMessage(const char* line1, const char* line2) {
    if (!ok) return;
    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setCursor(0, 4);  oled.print(line1);
    oled.setCursor(0, 20); oled.print(line2);
    oled.display();
    messageUntil = millis() + 1500;
}

// Page 0: how to connect.  Hotspot mode shows name + password + address.
static void pageStatus() {
    if (webIsHotspot()) {
        oled.setCursor(0, 0);  oled.print("WiFi: "); oled.print(webSsid());
        oled.setCursor(0, 8);
        oled.print(settingsPasswordCustom() ? "Pass: (your password)" : "Pass: " ROBOT_PASSWORD);
        oled.setCursor(0, 16); oled.print("http://"); oled.print(webAddress());
    } else {
        oled.setCursor(0, 0);  oled.print("WiFi: "); oled.print(webSsid());
        oled.setCursor(0, 8);  oled.print("http://"); oled.print(webAddress());
        oled.setCursor(0, 16); oled.print(settingsHostname()); oled.print(".local");
    }
    oled.setCursor(0, 24);
    oled.printf("%s %s", modeName(), motors.enabled() ? "" : "(stby)");
}

// Page 1: motor speed (%) and encoder count / speed
static void pageEncoders() {
    oled.setCursor(0, 0);
    oled.printf("Motors  %s", motors.enabled() ? "on" : "standby");
    oled.setCursor(0, 12);
    oled.printf("A%+4d%% %6ld %4.0f%s", (int)motors.A.speed(), encoderCount(0),
                encoderRate(0), encoderRateUnit());
    oled.setCursor(0, 22);
    oled.printf("B%+4d%% %6ld %4.0f%s", (int)motors.B.speed(), encoderCount(1),
                encoderRate(1), encoderRateUnit());
}

static void pageLine() {
    uint8_t mask = lineRead();
    oled.setCursor(0, 0);
    float pos;
    if (linePosition(mask, pos)) oled.printf("Line  pos %+.2f", pos);
    else                         oled.print("Line  (none)");
    for (uint8_t i = 0; i < 8; i++) {                     // 8 boxes, 16 px apart
        int x = i * 16 + 2;
        if (mask & (1 << i)) oled.fillRect(x, 14, 12, 16, SSD1306_WHITE);
        else                 oled.drawRect(x, 14, 12, 16, SSD1306_WHITE);
    }
}

static void pageInputs() {
    oled.setCursor(0, 0);
    oled.printf("SW1:%d SW2:%d BOOT:%d", buttonDown(BTN_SW1), buttonDown(BTN_SW2), buttonDown(BTN_BOOT));
    oled.setCursor(0, 12);
    oled.printf("Servo1:%4d Servo2:%4d", servoAngle(1), servoAngle(2));
    oled.setCursor(0, 24);
    oled.printf("Chip %.1fC  v%s", temperatureRead(), FW_VERSION);
}

void displayUpdate() {
    static uint32_t last = 0;
    if (!ok || millis() - last < 200 || millis() < messageUntil) return;   // 5x a second
    last = millis();

    oled.clearDisplay();
    oled.setTextSize(1);
    switch (page) {
        case 0: pageStatus();   break;
        case 1: pageEncoders(); break;
        case 2: pageLine();     break;
        case 3: pageInputs();   break;
    }
    oled.display();
}
