#include "huskylens_reader.h"
#include <Wire.h>
#include <HUSKYLENS.h>

static HUSKYLENS husky;

bool huskyLensInit() {
    Wire.begin();   // default Nano ESP32 I2C pins (SDA=A4, SCL=A5)

    // Retry a few times — HuskyLens occasionally needs a moment after power-on
    for (int attempt = 0; attempt < 5; attempt++) {
        if (husky.begin(Wire)) {
            Serial.println("[HuskyLens] Connected");
            return true;
        }
        Serial.printf("[HuskyLens] Connect attempt %d failed\n", attempt + 1);
        delay(300);
    }
    Serial.println("[HuskyLens] Init failed — check wiring and I2C protocol mode");
    return false;
}

int huskyLensReadLargestTagId() {
    if (!husky.request()) {
        // Bus error or no response. Common during transient I2C glitches —
        // don't log every time, just return no-detection.
        return -1;
    }

    if (!husky.available()) {
        return -1;   // HuskyLens responded but sees nothing recognised
    }

    // Find the tag with the largest bounding box (closest to camera, biggest in frame)
    int bestId = -1;
    int32_t bestArea = 0;
    while (husky.available()) {
        HUSKYLENSResult r = husky.read();
        int32_t area = (int32_t)r.width * (int32_t)r.height;
        if (area > bestArea) {
            bestArea = area;
            bestId = r.ID;
        }
    }
    return bestId;
}