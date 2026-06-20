#include "buzzer.h"
#include "config.h"

static bool     beepActive  = false;
static uint32_t beepStartedAt = 0;
static uint32_t beepDuration  = 0;

void buzzerInit() {
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
}

void buzzerBeep(uint32_t durationMs) {
    digitalWrite(BUZZER_PIN, HIGH);
    beepActive    = true;
    beepStartedAt = millis();
    beepDuration  = durationMs;
}

void buzzerUpdate() {
    if (!beepActive) return;
    if (millis() - beepStartedAt >= beepDuration) {
        digitalWrite(BUZZER_PIN, LOW);
        beepActive = false;
    }
}