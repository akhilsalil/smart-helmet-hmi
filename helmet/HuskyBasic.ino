//This is a sample code to test the proper functioning of the arduino with the huskylens
#include "HUSKYLENS.h"
#include "Wire.h"

HUSKYLENS huskylens;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // Initialize HuskyLens via I2C
  while (!huskylens.begin(Wire)) {
    Serial.println("Searching for HuskyLens...");
  }
  
}

void loop() {
  // 1. Ask HuskyLens for current data
  if (!huskylens.request()) {
    return;
  }

  // 2. Process results
  if (huskylens.isLearned() && huskylens.available()) {
    
    while (huskylens.available()) {
      HUSKYLENSResult result = huskylens.read();
      
      
      if (result.ID == 1) {
        Serial.println("det");
        delay(100); 
      }
      else if (result.ID == 2){
        Serial.println("str");
      }
    }
  }
  else{
    Serial.println("Lens not found");
  }
}