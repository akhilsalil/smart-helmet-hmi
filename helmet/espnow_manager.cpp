// NOTE: This helmet runs on Arduino's ESP32 core 2.0.18-arduino.5 (frozen
// for Nano ESP32). Callbacks use the OLD signatures. The wrist uses NEW
// (core 3.x). Do not copy callbacks between the two without checking.

#include "espnow_manager.h"
#include "config.h"
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>

static uint8_t wristMac[] = WRIST_MAC;

// Pending scan state — set by recv callback, consumed by loop()
static bool     scanPending     = false;
static uint32_t scanRequestedAt = 0;
static const uint32_t SCAN_STUB_DELAY_MS = 2000;
static const uint8_t  STUB_ROBOT_ID      = 2;   // Amazon bot — has command buttons

static void onDataRecv(const uint8_t *mac, const uint8_t *data, int len) {
    if (len != sizeof(EspNowMessage)) {
        Serial.printf("[ESP-NOW] Bad packet size: %d (expected %d)\n",
                      len, sizeof(EspNowMessage));
        return;
    }
    EspNowMessage msg;
    memcpy(&msg, data, sizeof(msg));
    Serial.printf("[ESP-NOW] Got msgType=%d robotId=%d from %02X:%02X:%02X:%02X:%02X:%02X\n",
                  msg.msgType, msg.robotId,
                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    if (msg.msgType == ESPNOW_MSG_SCAN_REQUEST) {
        // Don't send from inside the callback. Set a flag, let loop() handle it
        // after the simulated delay.
        scanPending     = true;
        scanRequestedAt = millis();
        Serial.println("[Scan] Request received, will respond after stub delay");
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
    return true;
}

bool espNowSendToWrist(const EspNowMessage &msg) {
    esp_err_t result = esp_now_send(wristMac, (const uint8_t *)&msg, sizeof(msg));
    return result == ESP_OK;
}

void espNowProcessPendingScan() {
    if (!scanPending) return;
    if (millis() - scanRequestedAt < SCAN_STUB_DELAY_MS) return;

    // Stub: pretend HuskyLens detected the Amazon bot.
    // When HuskyLens is wired, this is where we'd call huskyLensRead()
    // and use the actual recognised ID. On no-match, send SCAN_FAILED.
    EspNowMessage reply = {};
    reply.msgType = ESPNOW_MSG_SCAN_RESULT;
    reply.robotId = STUB_ROBOT_ID;
    Serial.printf("[Scan] Sending stubbed result: robotId=%d\n", reply.robotId);
    espNowSendToWrist(reply);

    scanPending = false;
}