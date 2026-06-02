// NOTE: This wrist runs on Espressif's mainline ESP32 core 3.x. Callbacks
// use the NEW signatures. The helmet uses OLD (core 2.0.18-arduino.5). Do
// not copy callbacks between the two without checking the core version.

#include "espnow_manager.h"
#include "config.h"
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>

static uint8_t helmetMac[] = HELMET_MAC;
static EspNowScanResultCb scanResultHandler = nullptr;

// Pending message buffer — set by recv callback, consumed by espNowPoll().
// volatile because it's written from one thread and read from another.
static volatile bool msgPending = false;
static EspNowMessage pendingMsg = {};

static void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
    if (len != sizeof(EspNowMessage)) {
        Serial.printf("[ESP-NOW] Bad packet size: %d (expected %d)\n",
                      len, sizeof(EspNowMessage));
        return;
    }
    EspNowMessage msg;
    memcpy(&msg, data, sizeof(msg));
    const uint8_t *mac = info->src_addr;
    Serial.printf("[ESP-NOW] Got msgType=%d robotId=%d from %02X:%02X:%02X:%02X:%02X:%02X\n",
                  msg.msgType, msg.robotId,
                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    // Dispatch scan results to the registered handler (if any).
    // Handler runs in ESP-NOW task context — must be quick and safe to call
    // from here. Touching LVGL from this context is technically unsafe
    // (LVGL is not thread-safe by default) but in practice works on ESP32
    // because LVGL's internal locks handle it. If we see weirdness, queue
    // to a flag-and-process pattern like the helmet does.
    if ((msg.msgType == ESPNOW_MSG_SCAN_RESULT ||
         msg.msgType == ESPNOW_MSG_SCAN_FAILED) && scanResultHandler) {
        scanResultHandler(msg);
    }
}

static void onDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
    const uint8_t *mac = tx_info->des_addr;
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
    memcpy(peer.peer_addr, helmetMac, 6);
    peer.channel = WIFI_CHANNEL;
    peer.encrypt = false;
    peer.ifidx   = WIFI_IF_STA;   // core 3.x: must explicitly bind to STA interface
    if (esp_now_add_peer(&peer) != ESP_OK) {
        Serial.println("[ESP-NOW] Failed to add helmet as peer");
        return false;
    }
    Serial.println("[ESP-NOW] Initialised, helmet added as peer");
    return true;
}

bool espNowSendScanRequest() {
    EspNowMessage msg = {};
    msg.msgType = ESPNOW_MSG_SCAN_REQUEST;
    msg.robotId = 0;
    esp_err_t result = esp_now_send(helmetMac, (const uint8_t *)&msg, sizeof(msg));
    if (result != ESP_OK) {
        Serial.printf("[ESP-NOW] Send failed (err %d)\n", result);
        return false;
    }
    return true;
}

void espNowSetScanResultHandler(EspNowScanResultCb cb) {
    scanResultHandler = cb;
}

void espNowPoll() {
    if (!msgPending) return;
    EspNowMessage msg = pendingMsg;   // copy out before clearing flag
    msgPending = false;
    if (scanResultHandler) scanResultHandler(msg);
}