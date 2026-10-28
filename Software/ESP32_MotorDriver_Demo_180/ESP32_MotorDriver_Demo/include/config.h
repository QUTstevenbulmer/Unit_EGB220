// =============================================================================
//  config.h  -  Settings you are encouraged to change.
// =============================================================================
#pragma once

// ---------------------------------------------------------------------------
//  WiFi
//  With no network saved the board makes its OWN hotspot. Join it from a
//  phone - the control page pops up (or browse to http://192.168.4.1).
//
//  Save up to WIFI_MAX_NETWORKS networks (e.g. QUT, home, phone hotspot)
//  from "WiFi setup" on the web page, or "wifi set" in the serial monitor.
//  At power-up the robot scans and tries each saved network that is in
//  range, in list order. If none work it makes its own hotspot.
//  WIFI_STA_SSID/PASSWORD below are only a default for a brand-new board.
//  Hold SW1 while powering on to forget all saved networks.
// ---------------------------------------------------------------------------
//
//  Robot name: every board gets a unique name, "motordriver-" + the last 4
//  hex digits of its MAC (e.g. motordriver-1a2b). The name is used for BOTH
//  the hotspot name and the web address http://<name>.local
//  Give each robot its own name (e.g. "team3") from the web page's
//  "Robot setup" card or the serial command  name team3
// ---------------------------------------------------------------------------
#define DEFAULT_HOSTNAME      "motordriver"    // + "-" + 4 MAC digits
#define WIFI_STA_SSID         ""               // leave "" for access-point mode
#define WIFI_STA_PASSWORD     ""
#define WIFI_STA_TIMEOUT_MS   10000   // per network (enterprise gets 20 s)
#define WIFI_MAX_NETWORKS     5       // how many networks can be saved
#define WIFI_LOST_RESTART_S   60      // network gone this long (and robot idle)?
                                      // restart to re-scan / make the hotspot. 0 = off

// ---------------------------------------------------------------------------
//  Over-the-air (OTA) update - upload new code over WiFi instead of USB.
//    * Web page: "Firmware update" card - pick a .bin file
//    * PlatformIO: the [env:ota] section in platformio.ini
//    * Arduino IDE: Tools > Port > (network port named after the robot)
//  OTA uses the robot password (below).
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
//  Robot password - ONE password for:
//    * logging in to the web pages (asked once per browser session)
//    * the robot's own hotspot WiFi
//    * firmware updates (web page upload, PlatformIO / Arduino IDE OTA)
//  This is only the default: each team sets its own from the web page
//  (Robot setup) or with the serial command  password <new>
//  Forgotten it? Hold SW2 while powering on, or type  password default
//  over USB.  Must be 8+ characters (a WiFi rule).
// ---------------------------------------------------------------------------
#define ROBOT_PASSWORD        "motors123"
#define WEB_LOGIN_REQUIRED    true    // false = web pages open to anyone

// ---------------------------------------------------------------------------
//  Motors
// ---------------------------------------------------------------------------
#define MOTOR_PWM_FREQ_HZ     20000   // 20 kHz = above hearing, no whine
#define MOTOR_PWM_BITS        10      // 0..1023 duty steps
#define MOTOR_RAMP_MS         600     // time to go 0 -> 100 % (0 = instant)

// On the breakout's JST connectors (J5, J6) both motors are wired the same
// way round, so no inversion is needed. On a two-wheel robot the motors face
// opposite directions - reverse ONE of them so "forward" is forward.
// These are only the DEFAULTS: tick "Reverse" on the web page's Robot setup
// card (or type "rev a" / "rev b") - that choice is saved in flash.
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
#define ENC_A_INVERTED        false   // default; also "rev enca" / web page
#define ENC_B_INVERTED        false

// ---------------------------------------------------------------------------
//  Speed control (PID) - uses the encoders so "50 %" means half of top speed
//  and the robot drives straight. Turn on with "pid on" or the web pages.
//  Measure top speed first with "pid cal" (wheels off the ground).
//  These are the starting values - tune with "pid kp 0.8" etc (saved).
// ---------------------------------------------------------------------------
#define SPEED_MAX_CPS         0       // top speed, counts/s (0 = run pid cal)
#define SPEED_KP              0.6     // % power per % speed error
#define SPEED_KI              4.0     // removes steady error (per second)
#define SPEED_KD              0.0     // damping - usually not needed
#define SPEED_KS              8.0     // drive-straight strength

// ---------------------------------------------------------------------------
//  Servos (J8, J9)
// ---------------------------------------------------------------------------
#define SERVO_MIN_US          500     // pulse for 0 degrees
#define SERVO_MAX_US          2500    // pulse for 180 degrees
#define SERVO_1_INVERTED      false   // default; also "rev s1" / web page
#define SERVO_2_INVERTED      false

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

// ---------------------------------------------------------------------------
//  Web terminal: how much recent serial output (Log.print...) is kept for
//  the Terminal on the web pages.
// ---------------------------------------------------------------------------
#define LOG_BUFFER_BYTES      6144

// Firmware version shown in the banner, OLED and web page
#define FW_VERSION            "1.8.0"
