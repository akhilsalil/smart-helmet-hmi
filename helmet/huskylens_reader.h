#ifndef HUSKYLENS_READER_H
#define HUSKYLENS_READER_H

#include <Arduino.h>

// Initialise HuskyLens on the default I2C pins (Wire = SDA=A4, SCL=A5).
// Returns true on success. Blocks for a short retry loop if HuskyLens isn't responding.
bool huskyLensInit();

// Single non-blocking read. Returns the ID of the largest tag in view,
// or -1 if nothing recognised this frame.
// Call repeatedly from the scan polling loop in espnow_manager.cpp.
int huskyLensReadLargestTagId();

#endif