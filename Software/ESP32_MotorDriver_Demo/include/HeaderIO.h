// =============================================================================
//  HeaderIO.h  -  The controller's spare GPIO headers (J4, J6, J7) and what
//                 the breakout board connects to each pin. Plus raw pin tests.
// =============================================================================
#pragma once
#include <Arduino.h>

struct HeaderPin {
    uint8_t     gpio;
    const char* header;   // controller header: "J4", "J6", "J7"
    uint8_t     pin;      // pin number on that header
    const char* bob;      // what the breakout board uses it for
};

extern const HeaderPin HEADER_PINS[];
extern const size_t    HEADER_PIN_COUNT;

const HeaderPin* findHeaderPin(uint8_t gpio);

void headerPrintTable();                      // show the pin map
void headerBlink(uint8_t gpio, uint32_t ms);  // toggle one pin for ms
void headerBlinkAll(uint32_t msEach);         // walk through every pin
void headerReadDigital();                     // read all pins as inputs
void i2cScan();                               // list I2C devices found
