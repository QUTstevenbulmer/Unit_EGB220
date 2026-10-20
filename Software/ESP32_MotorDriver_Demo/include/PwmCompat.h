// =============================================================================
//  PwmCompat.h  -  The ESP32 PWM ("LEDC") API changed between Arduino-ESP32
//  core 2.x and 3.x. These helpers work on both.
//
//  LEDC channel plan (ESP32-S2 has 8 channels; pairs share a timer/frequency):
//      0, 1   motors   20 kHz   (TB6612 library)
//      2, 3   servos   50 Hz
//      4..7   LEDs      5 kHz
// =============================================================================
#pragma once
#include <Arduino.h>
#if __has_include(<esp_arduino_version.h>)
#include <esp_arduino_version.h>
#endif

#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
inline void pwmAttach(uint8_t pin, uint8_t /*ch*/, uint32_t freq, uint8_t bits) { ledcAttach(pin, freq, bits); }
inline void pwmWrite(uint8_t pin, uint8_t /*ch*/, uint32_t duty)                { ledcWrite(pin, duty); }
#else
inline void pwmAttach(uint8_t pin, uint8_t ch, uint32_t freq, uint8_t bits) { ledcSetup(ch, freq, bits); ledcAttachPin(pin, ch); }
inline void pwmWrite(uint8_t /*pin*/, uint8_t ch, uint32_t duty)          { ledcWrite(ch, duty); }
#endif
