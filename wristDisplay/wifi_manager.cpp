#include "wifi_manager.h"
#include "config.h"
#include <WiFi.h>
#include <HTTPClient.h>

bool wifiConnect(const char* ssid, const char* password, uint32_t timeoutMs) {
    WiFi.begin(ssid, password);

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
    Serial.print("WRIST MAC: ");
    Serial.println(WiFi.macAddress());
    return true;
}

bool wifiConnected() {
    return WiFi.status() == WL_CONNECTED;
}

int httpGet(const char* url, char* responseBody, size_t bufferSize) {
    if (!wifiConnected()) return -1;

    HTTPClient http;
    http.begin(url);
    int statusCode = http.GET();

    if (statusCode > 0) {
        String body = http.getString();
        strncpy(responseBody, body.c_str(), bufferSize - 1);
        responseBody[bufferSize - 1] = '\0';
    } else {
        Serial.printf("[HTTP GET] Error: %s\n", http.errorToString(statusCode).c_str());
        responseBody[0] = '\0';
    }

    http.end();
    return statusCode;
}

int httpPost(const char* url, const char* jsonBody, char* responseBody, size_t bufferSize) {
    if (!wifiConnected()) return -1;

    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    int statusCode = http.POST(jsonBody);

    if (statusCode > 0) {
        String body = http.getString();
        strncpy(responseBody, body.c_str(), bufferSize - 1);
        responseBody[bufferSize - 1] = '\0';
    } else {
        Serial.printf("[HTTP POST] Error: %s\n", http.errorToString(statusCode).c_str());
        responseBody[0] = '\0';
    }

    http.end();
    return statusCode;
}
