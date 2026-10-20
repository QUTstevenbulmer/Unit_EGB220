#include "HeaderIO.h"
#include <Wire.h>
#include "pins.h"

// Controller header pin  ->  breakout board function (from both schematics)
const HeaderPin HEADER_PINS[] = {
    // gpio header pin   breakout use
    { 17, "J4",  1, "Line sensor LF_6"   },
    { 18, "J4",  2, "Line sensor LF_5"   },
    {  8, "J4",  3, "Button SW1"         },
    {  3, "J4",  4, "Encoder A - ch B"   },
    {  9, "J4",  5, "Encoder A - ch A"   },

    {  4, "J6",  2, "LED red"            },
    {  5, "J6",  3, "LED green"          },
    {  6, "J6",  4, "LED blue"           },
    {  7, "J6",  5, "LED yellow"         },
    { 15, "J6",  6, "Line sensor LF_8"   },
    { 16, "J6",  7, "Line sensor LF_7"   },

    {  1, "J7",  2, "Servo 1 (J8)"       },
    {  2, "J7",  3, "Servo 2 (J9)"       },
    { 43, "J7",  4, "UART TX (J14)"      },
    { 44, "J7",  5, "UART RX (J14)"      },
    { 42, "J7",  6, "Line sensor LF_1"   },
    { 41, "J7",  7, "Line sensor LF_2"   },
    { 40, "J7",  8, "Line sensor LF_3"   },
    { 39, "J7",  9, "Line sensor LF_4"   },
    { 34, "J7", 12, "I2C SDA (OLED)"     },
    { 33, "J7", 13, "I2C SCL (OLED)"     },
    { 13, "J7", 14, "Button SW2"         },
    { 12, "J7", 15, "Encoder B - ch A"   },
    { 11, "J7", 16, "Encoder B - ch B"   },
};
const size_t HEADER_PIN_COUNT = sizeof(HEADER_PINS) / sizeof(HEADER_PINS[0]);

const HeaderPin* findHeaderPin(uint8_t gpio) {
    for (size_t i = 0; i < HEADER_PIN_COUNT; i++)
        if (HEADER_PINS[i].gpio == gpio) return &HEADER_PINS[i];
    return nullptr;
}

void headerPrintTable() {
    Serial.println(F("\n  Ctrl header  Pin  GPIO  Breakout board use"));
    Serial.println(F("  -----------  ---  ----  ------------------"));
    for (size_t i = 0; i < HEADER_PIN_COUNT; i++) {
        const HeaderPin& p = HEADER_PINS[i];
        Serial.printf("  %-11s  %3u  IO%-2u  %s\n", p.header, p.pin, p.gpio, p.bob);
    }
    Serial.println(F("  Motor pins: PWMA 10, AIN1 21, AIN2 14, PWMB 38, BIN1 36, BIN2 37, STBY 35"));
}

// NOTE: these raw tests take a pin away from its normal job until reboot.
void headerBlink(uint8_t gpio, uint32_t ms) {
    pinMode(gpio, OUTPUT);
    uint32_t start = millis();
    while (millis() - start < ms) {
        digitalWrite(gpio, HIGH); delay(250);
        digitalWrite(gpio, LOW);  delay(250);
    }
    pinMode(gpio, INPUT);
}

void headerBlinkAll(uint32_t msEach) {
    Serial.println(F("Blinking each header pin in turn (LED + 330R to GND, or a meter)."));
    Serial.println(F("Press any key to stop. Reboot afterwards to restore normal use."));
    for (size_t i = 0; i < HEADER_PIN_COUNT; i++) {
        const HeaderPin& p = HEADER_PINS[i];
        Serial.printf("  %s pin %-2u (IO%u)  %s\n", p.header, p.pin, p.gpio, p.bob);
        headerBlink(p.gpio, msEach);
        if (Serial.available()) { while (Serial.available()) Serial.read(); break; }
    }
    Serial.println(F("Done."));
}

void headerReadDigital() {
    Serial.println(F("Raw digital levels of every header pin:"));
    for (size_t i = 0; i < HEADER_PIN_COUNT; i++) {
        const HeaderPin& p = HEADER_PINS[i];
        Serial.printf("  %s-%-2u IO%-2u = %d   %s\n", p.header, p.pin, p.gpio,
                      digitalRead(p.gpio), p.bob);
    }
}

// Uses the Wire bus already started in setup() (shared with the OLED).
void i2cScan() {
    Serial.printf("Scanning I2C on SDA=IO%u SCL=IO%u ...\n", PIN_I2C_SDA, PIN_I2C_SCL);
    int found = 0;
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf("  found device at 0x%02X%s\n", addr,
                          addr == 0x3C ? "  (OLED display)" : "");
            found++;
        }
    }
    if (!found) Serial.println(F("  nothing found"));
}
