#ifndef ESPNOW_MANAGER_H
#define ESPNOW_MANAGER_H

#include <Arduino.h>

// ---------------------------------------------------------------------------
// PROTOCOL — must match the wrist's espnow_manager.h exactly.
// If you change this struct, change it on BOTH boards.
// ---------------------------------------------------------------------------
typedef struct {
    uint8_t msgType;   // 1=scan_request, 2=scan_result, 3=scan_failed
    uint8_t robotId;   // only valid when msgType==2
} __attribute__((packed)) EspNowMessage;

#define ESPNOW_MSG_SCAN_REQUEST  1
#define ESPNOW_MSG_SCAN_RESULT   2
#define ESPNOW_MSG_SCAN_FAILED   3

bool espNowInit();

bool espNowSendToWrist(const EspNowMessage &msg);

// Called from loop() — processes any pending scan request after stub delay.
// Replace with real HuskyLens-driven scan when hardware is wired.
void espNowProcessPendingScan();

#endif