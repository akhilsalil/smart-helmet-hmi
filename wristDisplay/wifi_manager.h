#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>

// Connect to WiFi. Blocks until connected or timeout (ms).
// Returns true if connected.
bool wifiConnect(const char* ssid, const char* password, uint32_t timeoutMs = 10000);

bool wifiConnected();

// HTTP GET — fills responseBody buffer. Returns HTTP status code.
int httpGet(const char* url, char* responseBody, size_t bufferSize);

// HTTP POST with JSON body. Returns HTTP status code.
int httpPost(const char* url, const char* jsonBody, char* responseBody, size_t bufferSize);

#endif // WIFI_MANAGER_H
