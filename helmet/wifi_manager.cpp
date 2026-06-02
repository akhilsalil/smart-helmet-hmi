#include "wifi_manager.h"
#include <WiFi.h>

bool wifiConnect(const char* ssid, const char* password, uint32_t timeoutMs) {
    WiFi.mode(WIFI_STA);   // station mode (client, not AP)
    WiFi.begin(ssid, password);
    WiFi.setSleep(false);

    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start > timeoutMs) {
            Serial.println("[WiFi] Connection timed out");
            return false;
        }
        delay(200);
    }

    Serial.print("[WiFi] Connected, IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("[WiFi] Channel: ");
    Serial.println(WiFi.channel());
    return true;
}

bool wifiConnected() {
    return WiFi.status() == WL_CONNECTED;
}