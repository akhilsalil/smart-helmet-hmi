#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>

// Connect to WiFi. Blocks until connected or timeout. Returns true on success.
bool wifiConnect(const char* ssid, const char* password, uint32_t timeoutMs = 15000);

// True if currently associated with the AP.
bool wifiConnected();

#endif