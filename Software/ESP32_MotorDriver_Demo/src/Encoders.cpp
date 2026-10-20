// =============================================================================
//  Encoders.cpp  -  Quadrature encoders on the motor connectors J5 and J6.
//
//  An encoder gives two square waves, A and B, a quarter-cycle apart.
//  Which one changes first tells us the direction. We look at BOTH edges of
//  BOTH channels ("x4 decoding") using interrupts, so no pulse is missed.
//
//      old AB -> new AB      table below gives +1, -1 or 0 (no/invalid move)
// =============================================================================
#include "Breakout.h"
#include "config.h"
#include "pins.h"
#include "soc/gpio_reg.h"

struct Quad {
    volatile int32_t count = 0;
    volatile uint8_t state = 0;     // last A,B as 2 bits
    uint8_t pinA = 0, pinB = 0;
    int32_t lastCount = 0;
    float   rate = 0;               // smoothed counts/s
};

static Quad enc[2];
static const int8_t DRAM_ATTR QDEC[16] = { 0, -1, +1, 0,  +1, 0, 0, -1,  -1, 0, 0, +1,  0, +1, -1, 0 };

// Reading the GPIO input register directly is fast and safe inside an
// interrupt. (All four encoder pins are below 32, so one register covers them.)
static inline uint8_t IRAM_ATTR readAB(const Quad& q) {
    uint32_t in = REG_READ(GPIO_IN_REG);
    return (((in >> q.pinA) & 1) << 1) | ((in >> q.pinB) & 1);
}

static void IRAM_ATTR onEdge(void* arg) {
    Quad& q = *static_cast<Quad*>(arg);
    uint8_t now = readAB(q);
    q.count += QDEC[(q.state << 2) | now];
    q.state = now;
}

static void setupEncoder(Quad& q, uint8_t a, uint8_t b) {
    q.pinA = a; q.pinB = b;
    pinMode(a, INPUT_PULLUP);      // most motor encoders have open outputs
    pinMode(b, INPUT_PULLUP);
    q.state = readAB(q);
    attachInterruptArg(digitalPinToInterrupt(a), onEdge, &q, CHANGE);
    attachInterruptArg(digitalPinToInterrupt(b), onEdge, &q, CHANGE);
}

void encodersBegin() {
    setupEncoder(enc[0], PIN_ENC_A_A, PIN_ENC_A_B);
    setupEncoder(enc[1], PIN_ENC_B_A, PIN_ENC_B_B);
}

long encoderCount(uint8_t m) {
    if (m > 1) return 0;
    long c = enc[m].count;
    bool inv = (m == 0) ? ENC_A_INVERTED : ENC_B_INVERTED;
    return inv ? -c : c;
}

void encodersReset() {
    for (auto& q : enc) { noInterrupts(); q.count = 0; interrupts(); q.lastCount = 0; q.rate = 0; }
}

// Work out speed 10 times a second.
void encodersUpdate() {
    static uint32_t last = 0;
    uint32_t now = millis();
    uint32_t dt = now - last;
    if (dt < 100) return;
    last = now;
    for (uint8_t m = 0; m < 2; m++) {
        int32_t c = encoderCount(m);
        float cps = (c - enc[m].lastCount) * 1000.0f / dt;
        enc[m].lastCount = c;
        enc[m].rate = 0.6f * enc[m].rate + 0.4f * cps;       // light smoothing
    }
}

float encoderRate(uint8_t m) {
    if (m > 1) return 0;
#if ENC_COUNTS_PER_REV > 0
    return enc[m].rate * 60.0f / ENC_COUNTS_PER_REV;
#else
    return enc[m].rate;
#endif
}

const char* encoderRateUnit() { return ENC_COUNTS_PER_REV > 0 ? "rpm" : "cps"; }
