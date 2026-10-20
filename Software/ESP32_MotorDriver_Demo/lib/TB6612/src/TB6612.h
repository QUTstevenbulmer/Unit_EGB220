// =============================================================================
//  TB6612  -  Small, readable driver for the Toshiba TB6612FNG dual H-bridge.
//
//  How the chip works (one channel):
//
//      IN1  IN2  PWM   ->  result
//      ---  ---  ---       ------------------------------------------
//       H    L   PWM       forward,  speed = PWM duty cycle
//       L    H   PWM       reverse,  speed = PWM duty cycle
//       H    H    x        short brake (motor terminals shorted - stops fast)
//       L    L    x        coast / stop (outputs off - motor freewheels)
//      STBY = LOW          whole chip asleep, both motors off
//
//  Speeds in this library are in percent: -100 (full reverse) .. +100.
//  Changes are "ramped" so motors speed up / slow down smoothly - this is
//  kinder to gearboxes and stops big current spikes on the battery.
// =============================================================================
#pragma once
#include <Arduino.h>

class TB6612Motor {
public:
    // ledcChannel is only used on Arduino-ESP32 core 2.x (ignored on 3.x)
    void begin(uint8_t pwmPin, uint8_t in1Pin, uint8_t in2Pin,
               uint8_t ledcChannel, uint32_t pwmFreqHz, uint8_t pwmBits);

    void  setTarget(int percent);     // smooth: ramps towards this speed
    void  setSpeedNow(int percent);   // jump straight to this speed
    void  coast();                    // outputs off, motor freewheels
    void  brake();                    // short brake, motor stops quickly
    void  setInverted(bool inverted); // swap direction in software
    void  setRampTime(uint32_t msFor100Percent);
    void  update();                   // call often from loop()

    int   target()    const { return _target; }
    float speed()     const { return _current; }   // actual (ramped) speed
    bool  isBraking() const { return _braking; }

private:
    void  apply(float percent);
    void  writeDuty(uint32_t duty);

    uint8_t  _pwm = 0, _in1 = 0, _in2 = 0, _ch = 0;
    uint32_t _maxDuty = 1023;
    bool     _inverted = false;
    bool     _braking = false;
    int      _target = 0;
    float    _current = 0;
    uint32_t _rampMs = 0;
    uint32_t _lastUpdate = 0;
};

class TB6612 {
public:
    TB6612Motor A;
    TB6612Motor B;

    void begin(uint8_t stbyPin);
    void enable(bool on);            // drive STBY high/low
    bool enabled() const { return _enabled; }
    void stopAll(bool useBrake = false);
    void setRampTime(uint32_t msFor100Percent);
    uint32_t rampTime() const { return _rampMs; }
    void update();

private:
    uint8_t  _stby = 0;
    bool     _enabled = false;
    uint32_t _rampMs = 0;
};
