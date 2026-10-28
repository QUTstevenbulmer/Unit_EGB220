// =============================================================================
//  Log.h  -  Print debug messages to the USB serial monitor AND the web page.
//
//  Use it exactly like Serial:
//      Log.println("Hello");
//      Log.printf("Left encoder: %ld\r\n", encoderCount(0));
//  The text goes to USB (as Serial would) and into a small buffer that the
//  web pages' Terminal shows - so you can debug over WiFi with no cable.
//  (End lines with \r\n so they also look right in MobaXterm / PuTTY.)
// =============================================================================
#pragma once
#include <Arduino.h>

class LogTee : public Print {
public:
    size_t write(uint8_t c) override;
    size_t write(const uint8_t* buf, size_t len) override;
    using Print::write;

    // For the web terminal: text written since position 'from'.
    // 'from' is updated to the new end. Pass a negative number to get
    // everything still in the buffer.
    String readSince(long& from);
};
extern LogTee Log;
