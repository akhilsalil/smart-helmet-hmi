#ifndef SAFETY_BUBBLE_H
#define SAFETY_BUBBLE_H

#include <Arduino.h>

// Initialise safety bubble state. Call once from setup().
void safetyBubbleInit();

// Called when a beacon arrives (from ESP-NOW or mock Serial command).
// robotId: which robot the beacon is from
// dangerLevel: 0=safe, 1=caution, 2=sensitive, 3=high danger
// rssi: signal strength in dBm (typically -30 to -90)
void safetyBubbleOnBeacon(uint8_t robotId, uint8_t dangerLevel, int8_t rssi);

// Called from loop(). Handles beacon timeouts and alarm hold logic.
void safetyBubbleUpdate();

#endif