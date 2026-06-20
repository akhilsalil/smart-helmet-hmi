// NOTE: This helmet runs on Arduino's ESP32 core 2.0.18-arduino.5 (frozen
// for Nano ESP32). Callbacks use the OLD signatures. The wrist uses NEW
// (core 3.x). Do not copy callbacks between the two without checking.

#include "espnow_manager.h"
#include "config.h"
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include "huskylens_reader.h"
#include "safety_bubble.h"

static uint8_t wristMac[] = WRIST_MAC;

// Pending scan state — set by recv callback, consumed by loop()
static bool     scanPending     = false;
static uint32_t scanRequestedAt = 0;
static const uint32_t SCAN_TIMEOUT_MS    = 3000;   // give up after 3s of no detection
static const uint32_t SCAN_POLL_INTERVAL = 100;    // poll HuskyLens every 100ms
static uint32_t       lastPollAt         = 0;

// Pending scanner beacon — set by recv callback, consumed by loop()
static bool          beaconPending    = false;
static ScannerBeacon pendingBeacon    = {};

static void onDataRecv(const uint8_t *mac, const uint8_t *data, int len) {
    if (len == sizeof(EspNowMessage)) {
        EspNowMessage msg;
        memcpy(&msg, data, sizeof(msg));
        Serial.printf("[ESP-NOW] Got msgType=%d robotId=%d\n",
                      msg.msgType, msg.robotId);
        if (msg.msgType == ESPNOW_MSG_SCAN_REQUEST) {
            scanPending     = true;
            scanRequestedAt = millis();
            lastPollAt      = 0;
            Serial.println("[Scan] Request received, polling HuskyLens");
        }
    } else if (len == sizeof(ScannerBeacon)) {
        ScannerBeacon beacon;
        memcpy(&beacon, data, sizeof(beacon));
        if (beacon.msgType == ESPNOW_MSG_SCANNER_BEACON) {
            pendingBeacon = beacon;
            beaconPending = true;
        }
    } else {
        // foreign traffic — silently ignore, no log spam
    }
}

static void onDataSent(const uint8_t *mac, esp_now_send_status_t status) {
    Serial.printf("[ESP-NOW] Send to %02X:%02X:%02X:%02X:%02X:%02X: %s\n",
                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
                  status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL");
}

bool espNowInit() {
    esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
    if (esp_now_init() != ESP_OK) {
        Serial.println("[ESP-NOW] Init failed");
        return false;
    }
    esp_now_register_recv_cb(onDataRecv);
    esp_now_register_send_cb(onDataSent);

    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, wristMac, 6);
    peer.channel = WIFI_CHANNEL;
    peer.encrypt = false;
    if (esp_now_add_peer(&peer) != ESP_OK) {
        Serial.println("[ESP-NOW] Failed to add wrist as peer");
        return false;
    }
    Serial.println("[ESP-NOW] Initialised, wrist added as peer");

    // Scanner board peer
    uint8_t scannerMac[] = { 0xB0, 0xCB, 0xD8, 0xC6, 0xAF, 0x68 };
    esp_now_peer_info_t scannerPeer = {};
    memcpy(scannerPeer.peer_addr, scannerMac, 6);
    scannerPeer.channel = WIFI_CHANNEL;
    scannerPeer.encrypt = false;
    if (esp_now_add_peer(&scannerPeer) != ESP_OK) {
        Serial.println("[ESP-NOW] Failed to add scanner as peer");
        return false;
    }

    return true;
}

bool espNowSendToWrist(const EspNowMessage &msg) {
    esp_err_t result = esp_now_send(wristMac, (const uint8_t *)&msg, sizeof(msg));
    return result == ESP_OK;
}

void espNowProcessPendingScan() {
    if (!scanPending) return;

    uint32_t elapsed = millis() - scanRequestedAt;

    // Timeout — no detection within window, send SCAN_FAILED
    if (elapsed >= SCAN_TIMEOUT_MS) {
        Serial.println("[Scan] Timeout — no tag detected");
        EspNowMessage reply = {};
        reply.msgType = ESPNOW_MSG_SCAN_FAILED;
        reply.robotId = 0;
        espNowSendToWrist(reply);
        scanPending = false;
        return;
    }

    // Throttle polling — only ask HuskyLens every SCAN_POLL_INTERVAL ms
    if (millis() - lastPollAt < SCAN_POLL_INTERVAL) return;
    lastPollAt = millis();

    int id = huskyLensReadLargestTagId();
    if (id < 0) return;   // nothing this poll, keep waiting

    // First detection wins. Send result back and clear scan state.
    Serial.printf("[Scan] HuskyLens detected tag ID=%d (after %lums)\n", id, elapsed);
    EspNowMessage reply = {};
    reply.msgType = ESPNOW_MSG_SCAN_RESULT;
    reply.robotId = (uint8_t)id;
    espNowSendToWrist(reply);
    scanPending = false;
}

void espNowProcessPendingBeacon() {
    if (!beaconPending) return;
    beaconPending = false;
    safetyBubbleOnBeacon(pendingBeacon.robotId,
                         pendingBeacon.dangerLevel,
                         pendingBeacon.rssi);
}

bool espNowSendAlarmState(bool active, uint8_t robotId, uint8_t dangerLevel) {
    AlarmStateMsg msg;
    msg.msgType     = ESPNOW_MSG_ALARM_STATE;
    msg.alarmActive = active ? 1 : 0;
    msg.robotId     = robotId;
    msg.dangerLevel = dangerLevel;

    Serial.printf("[ESP-NOW] Sending AlarmState: active=%d robotId=%d danger=%d to %02X:%02X:%02X:%02X:%02X:%02X\n",
                  msg.alarmActive, msg.robotId, msg.dangerLevel,
                  wristMac[0], wristMac[1], wristMac[2], wristMac[3], wristMac[4], wristMac[5]);


    esp_err_t result = esp_now_send(wristMac, (const uint8_t*)&msg, sizeof(msg));

    Serial.printf("[ESP-NOW] Send result: %s\n", result == ESP_OK ? "OK (queued)" : "FAILED");

    return result == ESP_OK;
}