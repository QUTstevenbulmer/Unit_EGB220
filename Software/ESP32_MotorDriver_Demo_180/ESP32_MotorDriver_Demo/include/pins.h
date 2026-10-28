// =============================================================================
//  pins.h  -  Every GPIO used by the controller AND the breakout board (BOB).
//
//  Taken from the KiCad schematics:
//      ESP32_MotorDriver.kicad_sch       (controller - ESP32-S2 + TB6612)
//      ESP32_MotorDriver_BOB.kicad_sch   (breakout the controller plugs into)
//
//  If the hardware changes, this is the ONLY file that should need editing.
// =============================================================================
#pragma once
#include <Arduino.h>

// =============================================================================
//  CONTROLLER BOARD
// =============================================================================

// ---- TB6612FNG dual motor driver -------------------------------------------
//  Motor A -> BOB J5 (JST, with encoder)  or  J2 pins 2/3 on the controller
//  Motor B -> BOB J6 (JST, with encoder)  or  J2 pins 5/4 on the controller
constexpr uint8_t PIN_MOTOR_A_PWM = 10;   // PWMA  (net M0_PWM)
constexpr uint8_t PIN_MOTOR_A_IN1 = 21;   // AIN1
constexpr uint8_t PIN_MOTOR_A_IN2 = 14;   // AIN2
constexpr uint8_t PIN_MOTOR_B_PWM = 38;   // PWMB  (net M1_PWM)
constexpr uint8_t PIN_MOTOR_B_IN1 = 36;   // BIN1
constexpr uint8_t PIN_MOTOR_B_IN2 = 37;   // BIN2
constexpr uint8_t PIN_MOTOR_STBY  = 35;   // STBY - 10k pull-down: motors OFF at boot

// ---- Buttons on the controller ---------------------------------------------
constexpr uint8_t PIN_BOOT_BUTTON = 0;    // SW2 "BOOT" (LOW = pressed)

// ---- Native USB - used by Serial / programming, don't touch ----------------
constexpr uint8_t PIN_USB_DM = 19;
constexpr uint8_t PIN_USB_DP = 20;

// =============================================================================
//  BREAKOUT BOARD (BOB)
// =============================================================================

// ---- LEDs (active HIGH, 150R to GND) ---------------------------------------
constexpr uint8_t PIN_LED_RED    = 4;     // D1
constexpr uint8_t PIN_LED_GREEN  = 5;     // D2
constexpr uint8_t PIN_LED_BLUE   = 6;     // D3
constexpr uint8_t PIN_LED_YELLOW = 7;     // D4

// ---- Push buttons (10k pull-up + RC filter, LOW = pressed) -----------------
constexpr uint8_t PIN_SW1 = 8;            // SW1
constexpr uint8_t PIN_SW2 = 13;           // SW2

// ---- Hobby servos (J8, J9: GND / +5V / signal) -----------------------------
constexpr uint8_t PIN_SERVO_1 = 1;        // J8
constexpr uint8_t PIN_SERVO_2 = 2;        // J9

// ---- Motor encoders (on the JST-XH motor connectors, powered from 3V3) -----
//  J5: 1 M1.1  2 M1.2  3 3V3  4 enc B  5 enc A  6 GND     (motor A)
//  J6: 1 M2.1  2 M2.2  3 3V3  4 enc B  5 enc A  6 GND     (motor B)
constexpr uint8_t PIN_ENC_A_A = 9;        // net IO9/enc_1A
constexpr uint8_t PIN_ENC_A_B = 3;        // net IO3/enc_1B
constexpr uint8_t PIN_ENC_B_A = 12;       // net IO12/enc_2A
constexpr uint8_t PIN_ENC_B_B = 11;       // net IO11/enc_2B

// ---- Line-follower header J15 (2x8: odd pins = signal, even pins = GND) ----
//  Index 0 = LF_1 (J15 pin 1) ... index 7 = LF_8 (J15 pin 15)
constexpr uint8_t PIN_LINE[8] = { 42, 41, 40, 39, 18, 17, 16, 15 };

// ---- I2C (3.3 V side; a PCA9306 shifts it to 5 V for the OLED and J14) -----
constexpr uint8_t PIN_I2C_SDA = 34;
constexpr uint8_t PIN_I2C_SCL = 33;

// ---- UART on J14 (2x3: GND, +5V, RXD, TXD, SDA_5V, SCL_5V) -----------------
constexpr uint8_t PIN_UART_TX = 43;       // TXD0 (3.3 V logic)
constexpr uint8_t PIN_UART_RX = 44;       // RXD0 (3.3 V logic)
