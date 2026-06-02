#include "config.h"
#include "wifi_manager.h"
#include "espnow_manager.h"
#include <WiFi.h>

void setup() {
    Serial.begin(115200);
    delay(500);   // give Serial Monitor a chance to attach
    Serial.println("\n=== Helmet arduino boot ===");

    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);   // buzzer off

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

    Serial.println("=== Ready ===");
}

void loop() {
    // Heartbeat — slow blink so we can see the helmet is alive
    static uint32_t lastBlink = 0;
    if (millis() - lastBlink > 1000) {
        lastBlink = millis();
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    }
    espNowProcessPendingScan();   //  handles deferred reply to scan requests
}