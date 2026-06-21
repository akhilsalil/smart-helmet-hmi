#ifndef VIB_MOTOR_H
#define VIB_MOTOR_H

#include <Arduino.h>

// Initialise the vibration motor GPIO. Call once from setup(), after
// pinMode/digitalWrite(LOW) groundwork already in wristDisplay.ino —
// or use this instead of that inline code (see integration note below).
void vibMotorInit();

// Call from loop(). Drives the motor according to the active pattern
// (single buzz / pulsing / continuous) and enforces the cutoff timer.
void vibMotorUpdate();

// Handler to register with espNowSetAlarmStateHandler(). Drives the motor
// state machine based on incoming AlarmStateMsg packets.
#include "espnow_manager.h"
void vibMotorOnAlarmState(const AlarmStateMsg &msg);

#endif