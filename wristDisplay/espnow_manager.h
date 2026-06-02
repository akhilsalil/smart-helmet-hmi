#ifndef ESPNOW_MANAGER_H
#define ESPNOW_MANAGER_H

#include <Arduino.h>

// ---------------------------------------------------------------------------
// PROTOCOL — must match the helmet's espnow_manager.h exactly.
// If you change this struct, change it on BOTH boards.
// ---------------------------------------------------------------------------
typedef struct {
    uint8_t msgType;   // 1=scan_request, 2=scan_result, 3=scan_failed
    uint8_t robotId;   // only valid when msgType==2
} __attribute__((packed)) EspNowMessage;

// Message type constants — kept in sync with the helmet
#define ESPNOW_MSG_SCAN_REQUEST  1
#define ESPNOW_MSG_SCAN_RESULT   2
#define ESPNOW_MSG_SCAN_FAILED   3

// Initialise ESP-NOW. Call AFTER WiFi connects. Adds the helmet as a peer.
// Returns true on success.
bool espNowInit();

// Send a scan-request packet to the helmet. Convenience wrapper.
// Returns true if the send was queued.
bool espNowSendScanRequest();

// Registered by screen_robot_list.cpp. Called from ESP-NOW receive context
// when a scan result/failure arrives. May be NULL if no handler registered.
// Signature receives the full message so the handler can act on msgType.
typedef void (*EspNowScanResultCb)(const EspNowMessage &msg);
void espNowSetScanResultHandler(EspNowScanResultCb cb);

// Call from loop(). Processes any pending received messages on the main
// thread. Required because ESP-NOW callbacks run in a separate task and
// LVGL is not thread-safe (caused wrist display to crash & reboot)
void espNowPoll();

#endif