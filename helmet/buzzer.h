#ifndef BUZZER_H
#define BUZZER_H

#include <Arduino.h>

// One-time setup: configures the buzzer pin as output, drives it low.
void buzzerInit();

// Trigger a single non-blocking beep of given duration.
// Safe to call repeatedly — calling during an active beep extends it.
// Call buzzerUpdate() from loop() to actually turn it off when done.
void buzzerBeep(uint32_t durationMs);

// Call from loop(). Turns the buzzer off once the beep duration has elapsed.
void buzzerUpdate();

#endif