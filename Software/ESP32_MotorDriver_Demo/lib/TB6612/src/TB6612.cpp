#include "TB6612.h"

#if __has_include(<esp_arduino_version.h>)
#include <esp_arduino_version.h>
#endif

// The LEDC (PWM) API changed between Arduino-ESP32 core 2.x and 3.x.
// These two helpers hide the difference so the rest of the code is the same.
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
  #define LEDC_V3 1
#else
  #define LEDC_V3 0
#endif

// ---------------------------------------------------------------- TB6612Motor
void TB6612Motor::begin(uint8_t pwmPin, uint8_t in1Pin, uint8_t in2Pin,
                        uint8_t ledcChannel, uint32_t pwmFreqHz, uint8_t pwmBits) {
    _pwm = pwmPin; _in1 = in1Pin; _in2 = in2Pin; _ch = ledcChannel;
    _maxDuty = (1UL << pwmBits) - 1;

    pinMode(_in1, OUTPUT);
    pinMode(_in2, OUTPUT);
    digitalWrite(_in1, LOW);   // start in "coast"
    digitalWrite(_in2, LOW);

#if LEDC_V3
    ledcAttach(_pwm, pwmFreqHz, pwmBits);
#else
    ledcSetup(_ch, pwmFreqHz, pwmBits);
    ledcAttachPin(_pwm, _ch);
#endif
    writeDuty(0);
    _lastUpdate = millis();
}

void TB6612Motor::writeDuty(uint32_t duty) {
#if LEDC_V3
    ledcWrite(_pwm, duty);
#else
    ledcWrite(_ch, duty);
#endif
}

void TB6612Motor::setInverted(bool inverted) { _inverted = inverted; }
void TB6612Motor::setRampTime(uint32_t ms)   { _rampMs = ms; }

void TB6612Motor::setTarget(int percent) {
    _target = constrain(percent, -100, 100);
    _braking = false;
}

void TB6612Motor::setSpeedNow(int percent) {
    setTarget(percent);
    _current = _target;
    apply(_current);
}

void TB6612Motor::coast() {
    _target = 0;
    _current = 0;
    _braking = false;
    apply(0);
}

void TB6612Motor::brake() {
    _target = 0;
    _current = 0;
    _braking = true;
    digitalWrite(_in1, HIGH);   // IN1 = IN2 = HIGH -> short brake
    digitalWrite(_in2, HIGH);
    writeDuty(0);
}

// Turn a speed in percent into IN1 / IN2 / PWM settings.
void TB6612Motor::apply(float percent) {
    if (_braking) return;                    // brake() owns the pins

    if (percent == 0) {                      // stopped -> coast
        digitalWrite(_in1, LOW);
        digitalWrite(_in2, LOW);
        writeDuty(0);
        return;
    }
    bool forward = (percent > 0) != _inverted;
    digitalWrite(_in1, forward ? HIGH : LOW);
    digitalWrite(_in2, forward ? LOW  : HIGH);
    writeDuty((uint32_t)(fabsf(percent) * _maxDuty / 100.0f + 0.5f));
}

// Move _current a little closer to _target, based on time since last call.
void TB6612Motor::update() {
    uint32_t now = millis();
    uint32_t dt  = now - _lastUpdate;
    _lastUpdate  = now;

    if (_braking || _current == _target) return;

    if (_rampMs == 0) {
        _current = _target;
    } else {
        float step = 100.0f * dt / _rampMs;  // % allowed to change this call
        float diff = _target - _current;
        if (fabsf(diff) <= step) _current = _target;
        else                     _current += (diff > 0 ? step : -step);
    }
    apply(_current);
}

// --------------------------------------------------------------------- TB6612
void TB6612::begin(uint8_t stbyPin) {
    _stby = stbyPin;
    pinMode(_stby, OUTPUT);
    enable(false);                // board has a pull-down too: safe at boot
}

void TB6612::enable(bool on) {
    _enabled = on;
    digitalWrite(_stby, on ? HIGH : LOW);
}

void TB6612::stopAll(bool useBrake) {
    if (useBrake) { A.brake(); B.brake(); }
    else          { A.coast(); B.coast(); }
}

void TB6612::setRampTime(uint32_t ms) {
    _rampMs = ms;
    A.setRampTime(ms);
    B.setRampTime(ms);
}

void TB6612::update() {
    A.update();
    B.update();
}
