//I have created the basic structure of code to start scanning using the huskylens after getting the trigger from the gauntlet and return the first object id that gets scanned.
#include "HUSKYLENS.h"
#include "Wire.h"


HUSKYLENS huskylens;

// State control variable
bool isScanning = false;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Wire.begin();
  delay(500);

  // Initialize HuskyLens via I2C
  while (!huskylens.begin(Wire)) {
    Serial.println("Searching for HuskyLens...");
    delay(500);  // Small pause to prevent serial flooding
  }
  Serial.println("HuskyLens Initialized and Ready.");
}

void loop() {

  // ========================================================
  // 1. USER INPUT CHECK (GATEKEEPER)
  // ========================================================
  if (!isScanning) {

    // --- PLACEHOLDER FOR USER INPUT ---
    // This boolean will represent your trigger condition.
    bool userInputTriggered = false;

    // SIMULATION EXAMPLE: Type any key into the Serial Monitor to trigger a scan
    if (Serial.available() > 0) {
      while (Serial.available() > 0) { Serial.read(); }  // Clear the buffer
      userInputTriggered = true;
    }

    // Replace the Serial check above with your Gauntlet ESP-NOW logic later:
    // if (espNowMessageReceived == true) { userInputTriggered = true; }

    if (userInputTriggered) {
      Serial.println("\n[SYSTEM ALERT] Scan initiated by user. Looking for targets...");
      isScanning = true;
    }
  }

  // ========================================================
  // 2. ACTIVE SCANNING LOOP
  // ========================================================
  if (isScanning) {
    // Request current data frame from the lens
    if (!huskylens.request()) {
      Serial.println("HuskyLens Request Failed.");
      return;
    }

    // Process results if the lens sees learned IDs
    if (huskylens.isLearned() && huskylens.available()) {

      if (huskylens.available()) {
        HUSKYLENSResult result = huskylens.read();
        delay(1000);

        if (result.ID < 3) {
          Serial.print("SUCCESS: Target Detected:");
          Serial.println(result.ID);

          // --- STOP SCANNING ONCE RECOGNIZED ---
          isScanning = false;
        }
      }

      // Provide visual feedback that the gate closed
      if (!isScanning) {
        Serial.println("[SYSTEM ALERT] Target confirmed. Lens tracking suspended.\n");
      }
    }
  }
}