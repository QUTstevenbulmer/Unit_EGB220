// =============================================================================
//  SpeedControl.h  -  Closed-loop wheel speed (PID) using the encoders.
//
//  OFF: a motor speed of 50 % means 50 % of the battery's power. A weaker
//       motor, a heavier side or a draggy wheel turns slower -> robot curves.
//  ON:  50 % means 50 % of TOP SPEED. Each wheel's real speed is measured
//       from its encoder 50 times a second and the power is adjusted until it
//       matches (PID). When both wheels are asked for the same speed, it also
//       compares how far each has turned and trims them back into step, so
//       the robot drives straight.
//
//  Before first use: wheels off the ground, then "pid cal" (measures top
//  speed), then "pid on". Settings are saved in flash.
// =============================================================================
#pragma once
#include <Arduino.h>

struct SpeedGains {
    float kp;      // % power per % speed error            (react to error now)
    float ki;      // % power per (% error x second)        (fix steady error)
    float kd;      // % power per (% speed change / second) (damp overshoot)
    float ks;      // % speed trim per (% x s) of distance gap between wheels
};

void  speedBegin();                          // load settings (after encodersBegin)
void  speedUpdate();                         // call from loop()
bool  speedEnabled();
bool  speedEnable(bool on, bool save = true);// false (with a message) if not calibrated
void  speedSetTarget(int a, int b);          // % of top speed, -100..100
int   speedTarget(uint8_t motor);            // what was asked for
float speedMeasured(uint8_t motor);          // actual speed, % of top speed
void  speedReset();                          // forget targets and history (on stop)
bool  speedCalibrate();                      // spins both wheels at 100 % ~2 s
float speedMaxCps();                         // top speed in counts/s (0 = not calibrated)
SpeedGains& speedGains();
void  speedSaveGains();
void  speedPrint();                          // settings to Log
void  speedSetLogging(bool on);              // print live values 10x a second
bool  speedLogging();
