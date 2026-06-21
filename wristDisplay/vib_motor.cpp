#include "vib_motor.h"

#define VIB_MOTOR_PIN 21

static const uint32_t LEVEL1_DURATION_MS   = 2000;  // single buzz
static const uint32_t LEVEL2_PULSE_ON_MS   = 500;
static const uint32_t LEVEL2_PULSE_OFF_MS  = 500;
static const uint32_t CUTOFF_MS            = 10000; // safety net if exit msg dropped

// Pattern state machine
enum VibPattern { VIB_NONE, VIB_SINGLE, VIB_PULSE, VIB_CONTINUOUS };

static VibPattern currentPattern   = VIB_NONE;
static bool       motorOn          = false;
static uint32_t   patternStartedAt = 0;   // when this alarm session started (for cutoff)
static uint32_t   pulsePhaseStartedAt = 0; // when the current on/off pulse phase started

static void motorWrite(bool on) {
    digitalWrite(VIB_MOTOR_PIN, on ? HIGH : LOW);
    motorOn = on;
}

void vibMotorInit() {
    pinMode(VIB_MOTOR_PIN, OUTPUT);
    motorWrite(false);
    currentPattern = VIB_NONE;
}

void vibMotorOnAlarmState(const AlarmStateMsg &msg) {
    if (msg.alarmActive) {
        patternStartedAt    = millis();
        pulsePhaseStartedAt = millis();

        switch (msg.dangerLevel) {
            case 1:
                currentPattern = VIB_SINGLE;
                motorWrite(true);   // single 2s buzz starts immediately
                break;
            case 2:
                currentPattern = VIB_PULSE;
                motorWrite(true);   // start with the "on" phase
                break;
            case 3:
                currentPattern = VIB_CONTINUOUS;
                motorWrite(true);
                break;
            default:
                // dangerLevel 0 or unrecognised — no alarm, don't vibrate
                currentPattern = VIB_NONE;
                motorWrite(false);
                break;
        }
    } else {
        // Explicit clear from helmet
        currentPattern = VIB_NONE;
        motorWrite(false);
    }
}

void vibMotorUpdate() {
    if (currentPattern == VIB_NONE) return;

    uint32_t elapsed = millis() - patternStartedAt;

    // Safety-net cutoff applies to all active patterns except the
    // already-self-limiting single buzz.
    if (currentPattern != VIB_SINGLE && elapsed >= CUTOFF_MS) {
        currentPattern = VIB_NONE;
        motorWrite(false);
        return;
    }

    switch (currentPattern) {
        case VIB_SINGLE:
            if (elapsed >= LEVEL1_DURATION_MS) {
                currentPattern = VIB_NONE;
                motorWrite(false);
            }
            break;

        case VIB_PULSE: {
            uint32_t phaseElapsed = millis() - pulsePhaseStartedAt;
            if (motorOn && phaseElapsed >= LEVEL2_PULSE_ON_MS) {
                motorWrite(false);
                pulsePhaseStartedAt = millis();
            } else if (!motorOn && phaseElapsed >= LEVEL2_PULSE_OFF_MS) {
                motorWrite(true);
                pulsePhaseStartedAt = millis();
            }
            break;
        }

        case VIB_CONTINUOUS:
            // Already on, nothing to do until cutoff or clear message
            break;

        case VIB_NONE:
            break;
    }
}