// =============================================================================
//  config.h  -  Settings you are encouraged to change.
// =============================================================================
#pragma once

// ---------------------------------------------------------------------------
//  WiFi
//  With no network saved the board makes its OWN hotspot. Join it from a
//  phone - the control page pops up (or browse to http://192.168.4.1).
//
//  To use your own WiFi, open "WiFi setup" on the web page, or type
//  "wifi set" in the serial monitor. The network is saved in flash.
//  WIFI_STA_SSID/PASSWORD below are only a default if nothing is saved.
//  If joining fails within WIFI_STA_TIMEOUT_MS it falls back to the hotspot.
//  Hold SW1 while powering on to forget the saved network.
// ---------------------------------------------------------------------------
#define WIFI_AP_SSID_PREFIX   "MotorDriver-"   // + last 4 hex digits of the MAC
#define WIFI_AP_PASSWORD      "motors123"      // min 8 characters
#define WIFI_STA_SSID         ""               // leave "" for access-point mode
#define WIFI_STA_PASSWORD     ""
#define WIFI_STA_TIMEOUT_MS   10000
#define MDNS_HOSTNAME         "motordriver"    // http://motordriver.local (STA mode)

// ---------------------------------------------------------------------------
//  Motors
// ---------------------------------------------------------------------------
#define MOTOR_PWM_FREQ_HZ     20000   // 20 kHz = above hearing, no whine
#define MOTOR_PWM_BITS        10      // 0..1023 duty steps
#define MOTOR_RAMP_MS         600     // time to go 0 -> 100 % (0 = instant)

// On the breakout's JST connectors (J5, J6) both motors are wired the same
// way round, so no inversion is needed. On a two-wheel robot the motors face
// opposite directions - set ONE of these to true so "forward" is forward.
#define MOTOR_A_INVERTED      false
#define MOTOR_B_INVERTED      false

// Safety: when driven from the web page, motors stop if the page goes quiet
// for this long (phone locked, WiFi dropped, tab closed...).
#define WEB_DEADMAN_MS        1000

// ---------------------------------------------------------------------------
//  Encoders
//  Counts per output-shaft revolution = encoder pulses x 4 x gear ratio.
//  e.g. N20 motor, 7-pulse encoder, 100:1 gearbox -> 7 x 4 x 100 = 2800.
//  Leave 0 if unknown - you will see counts/second instead of RPM.
// ---------------------------------------------------------------------------
#define ENC_COUNTS_PER_REV    0
#define ENC_A_INVERTED        false   // flip so +speed gives +counts
#define ENC_B_INVERTED        false

// ---------------------------------------------------------------------------
//  Servos (J8, J9)
// ---------------------------------------------------------------------------
#define SERVO_MIN_US          500     // pulse for 0 degrees
#define SERVO_MAX_US          2500    // pulse for 180 degrees

// ---------------------------------------------------------------------------
//  Line sensor (J15)
//  Most IR modules output HIGH over a black line. Flip if yours is opposite.
// ---------------------------------------------------------------------------
#define LINE_ACTIVE_HIGH      true
#define FOLLOW_BASE_SPEED     35      // % - cruising speed when line following
#define FOLLOW_KP             40      // steering strength (try 20..80)
#define FOLLOW_LOST_MS        400     // stop if the line is lost this long

// ---------------------------------------------------------------------------
//  OLED (0.91" 128x32 SSD1306 on the breakout)
// ---------------------------------------------------------------------------
#define OLED_I2C_ADDRESS      0x3C

// Firmware version shown in the banner, OLED and web page
#define FW_VERSION            "1.3.0"
