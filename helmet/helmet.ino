#include "config.h"
#include "wifi_manager.h"
#include "espnow_manager.h"
#include <WiFi.h>
#include "huskylens_reader.h"
#include "safety_bubble.h"
#include "buzzer.h"


// -----------------------------------------------------------------------------
// Serial command parser for mock beacons
// Usage: type "beacon <id> <danger> <rssi>" in Serial Monitor, e.g.:
//   beacon 2 1 -65    → robot 2, danger=CAUTION, rssi=-65
//   beacon 2 1 -85    → same robot, farther away
//   beacon 2 0 -65    → same robot, danger cleared
// Used during development before real ESP-NOW beacons are wired up.
// -----------------------------------------------------------------------------
static void handleSerialCommand() {
    if (!Serial.available()) return;
    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) return;

    if (line.startsWith("beacon")) {
        int id, danger, rssi;
        int parsed = sscanf(line.c_str(), "beacon %d %d %d", &id, &danger, &rssi);
        if (parsed == 3) {
            Serial.printf("[Mock] beacon id=%d danger=%d rssi=%d\n", id, danger, rssi);
            safetyBubbleOnBeacon((uint8_t)id, (uint8_t)danger, (int8_t)rssi);
        } else {
            Serial.println("Usage: beacon <id> <danger> <rssi>");
        }
    } else {
        Serial.printf("Unknown command: %s\n", line.c_str());
    }
}


// -----------------------------------------------------------------------------
// Reads beacon lines from the BLE scanner board over Serial1.
// Format: "robotId,danger,rssi"  e.g. "2,1,-65"
// -----------------------------------------------------------------------------
static void handleScannerLink() {
    if (!Serial1.available()) return;
    String line = Serial1.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) return;

    int id, danger, rssi;
    if (sscanf(line.c_str(), "%d,%d,%d", &id, &danger, &rssi) == 3) {
        safetyBubbleOnBeacon((uint8_t)id, (uint8_t)danger, (int8_t)rssi);
        Serial.printf("[Scanner] id=%d danger=%d rssi=%d\n", id, danger, rssi);
    }
}


void setup() {
    Serial.begin(115200);
    delay(2000);   // give Serial Monitor a chance to attach
    Serial.println("\n=== Helmet arduino boot ===");

    pinMode(LED_BUILTIN, OUTPUT);
    buzzerInit();

    // BLE scanner link. RX = D7 (the wire from the scanner). TX = D6 (unused).
    Serial1.begin(115200, SERIAL_8N1, D7, D6);
    Serial1.setTimeout(20);

    // Print our own MAC so we can verify it matches what the wrist has
    Serial.print("Helmet MAC: ");
    Serial.println(WiFi.macAddress());

    if (!wifiConnect(WIFI_SSID, WIFI_PASSWORD)) {
        Serial.println("WiFi failed — ESP-NOW will likely have channel issues.");
        // Continue anyway — ESP-NOW can work without WiFi if channel is set explicitly
    }

    if (!espNowInit()) {
        Serial.println("ESP-NOW init failed — halting");
        while (true) {
            digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
            delay(100);   // fast blink = error state
        }
    }

    if (!huskyLensInit()) {
    Serial.println("HuskyLens init failed — scans will always timeout. Continuing.");
    // Don't halt — wrist will get SCAN_FAILED on every scan, demo still partially works
    }

    safetyBubbleInit();

    Serial.println("=== Ready ===");
    Serial.println("Mock beacon commands: beacon <id> <danger> <rssi>");
}

void loop() {
    // Heartbeat — slow blink so we can see the helmet is alive
    static uint32_t lastBlink = 0;
    if (millis() - lastBlink > 1000) {
        lastBlink = millis();
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    }
    espNowProcessPendingScan();   //  handles deferred reply to scan requests
    espNowProcessPendingBeacon(); 
    safetyBubbleUpdate();
    buzzerUpdate();
    handleSerialCommand();
    handleScannerLink();
}