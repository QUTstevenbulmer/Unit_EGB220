// =============================================================================
//  pins.h  -  Every GPIO used by the controller AND the breakout board (BOB).
//
//  Taken from the KiCad schematics:
//      ESP32_MotorDriver.kicad_sch       (controller - ESP32-S2 + TB6612)
//      ESP32_MotorDriver_BOB.kicad_sch   (breakout board, revision v03)
//  BOB v03 moved the line sensor onto IO1-IO8 (analog) and shuffled the
//  LEDs, servos, SW1 and encoder A to make room.
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
constexpr uint8_t PIN_LED_RED    = 40;    // D1  (net IO40/LED_R)
constexpr uint8_t PIN_LED_GREEN  = 39;    // D2  (net IO39/LED_G)
constexpr uint8_t PIN_LED_BLUE   = 17;    // D3  (net IO17/LED_B)
constexpr uint8_t PIN_LED_YELLOW = 18;    // D4  (net IO18/LED_Y)

// ---- Push buttons (10k pull-up + RC filter, LOW = pressed) -----------------
constexpr uint8_t PIN_SW1 = 9;            // SW1 (net IO9/SW_1)
constexpr uint8_t PIN_SW2 = 13;           // SW2 (net IO13/SW_2)

// ---- Hobby servos (J8, J9: GND / +5V / signal) -----------------------------
constexpr uint8_t PIN_SERVO_1 = 42;       // J8 (net IO42/Servo_1)
constexpr uint8_t PIN_SERVO_2 = 41;       // J9 (net IO41/Servo_2)

// ---- Motor encoders (on the JST-XH motor connectors, powered from 3V3) -----
//  J5: 1 M1.1  2 3V3 (33R)  3 enc B  4 enc A  5 GND  6 M1.2   (motor A)
//  J6: 1 M2.2  2 3V3 (33R)  3 enc B  4 enc A  5 GND  6 M2.1   (motor B)
//  (J6's motor pins are the mirror of J5's - "Check motors & encoders" on
//   the web page sorts out which way each motor turns.)
constexpr uint8_t PIN_ENC_A_A = 16;       // net IO16/enc_1A
constexpr uint8_t PIN_ENC_A_B = 15;       // net IO15/enc_1B
constexpr uint8_t PIN_ENC_B_A = 12;       // net IO12/enc_2A
constexpr uint8_t PIN_ENC_B_B = 11;       // net IO11/enc_2B

// ---- Line-follower header J15 (Pololu QTR-8A) ------------------------------
//  J15 (2x8): odd pins = sensor outputs, pins 2-12 = GND, 14 = +3V3,
//  16 = LEDON (JP1, bridged to 3V3 = IR LEDs always on).
//      J15 pin: 1    3    5    7    9    11   13   15
//      net:     LF_8 LF_7 LF_6 LF_5 LF_4 LF_3 LF_2 LF_1
//      GPIO:    IO4  IO5  IO6  IO7  IO8  IO3  IO2  IO1    (all ADC1 = analog)
//
//  PIN_LINE is listed LEFT -> RIGHT as seen from above, robot facing forward:
//  the firmware treats index 0 as the left-most sensor. The bar is mounted
//  with LF_1 (its OUT1) on the robot's RIGHT, so the list starts with LF_8.
//  If you mount the bar the other way round, reverse this list.
constexpr uint8_t PIN_LINE[8] = { 4, 5, 6, 7, 8, 3, 2, 1 };   // LF_8 (left) ... LF_1 (right)
constexpr uint8_t LINE_LF_NUM[8] = { 8, 7, 6, 5, 4, 3, 2, 1 };  // J15 net name of each, for messages

// ---- I2C (3.3 V side; a PCA9306 shifts it to 5 V for the OLED and J14) -----
constexpr uint8_t PIN_I2C_SDA = 34;
constexpr uint8_t PIN_I2C_SCL = 33;

// ---- UART on J14 (2x3: GND, +5V, RXD, TXD, SDA_5V, SCL_5V) -----------------
constexpr uint8_t PIN_UART_TX = 43;       // TXD0 (3.3 V logic)
constexpr uint8_t PIN_UART_RX = 44;       // RXD0 (3.3 V logic)
