// Scans BLE, forwards beacons to Nano ESP32 over ESP-NOW.

#include <NimBLEDevice.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>

static uint8_t nanoMac[] = { 0xE4, 0xB0, 0x63, 0xAE, 0x05, 0x74 };

#define WIFI_CHANNEL 6

// Beacon packet sent to Nano — must match espnow_manager.h on the Nano exactly
typedef struct {
    uint8_t msgType;     // 20 = scanner beacon
    uint8_t robotId;
    uint8_t dangerLevel;
    int8_t  rssi;
} __attribute__((packed)) ScannerBeacon;

#define MSG_SCANNER_BEACON 20

static NimBLEScan* pScan = nullptr;

static void sendToNano(uint8_t robotId, uint8_t danger, int8_t rssi) {
    ScannerBeacon pkt;
    pkt.msgType    = MSG_SCANNER_BEACON;
    pkt.robotId    = robotId;
    pkt.dangerLevel = danger;
    pkt.rssi       = rssi;
    esp_now_send(nanoMac, (const uint8_t*)&pkt, sizeof(pkt));
}

class ScanCallbacks : public NimBLEScanCallbacks {
    void onResult(const NimBLEAdvertisedDevice* dev) override {
        if (!dev->haveManufacturerData()) return;
        std::string md = dev->getManufacturerData();
        if (md.length() != 5) return;
        if ((uint8_t)md[0] != 0xFF || (uint8_t)md[1] != 0xFF) return;
        if ((uint8_t)md[2] != 0x01) return;

        uint8_t robotId = (uint8_t)md[3];
        uint8_t danger  = (uint8_t)md[4];
        int8_t  rssi    = (int8_t)dev->getRSSI();

        Serial.printf("beacon id=%u danger=%u rssi=%d → sending ESP-NOW\n",
                      robotId, danger, rssi);
        sendToNano(robotId, danger, rssi);
    }

    void onScanEnd(const NimBLEScanResults& results, int reason) override {
        pScan->start(0, false);
    }
};

static ScanCallbacks scanCallbacks;

bool espNowInit() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    // Must explicitly set channel — without WiFi association,
    // board defaults to ch1 and will never reach the Nano on ch2 or 6
    esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);

    if (esp_now_init() != ESP_OK) {
        Serial.println("[ESP-NOW] Init failed");
        return false;
    }

    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, nanoMac, 6);
    peer.channel = WIFI_CHANNEL;
    peer.encrypt = false;
    peer.ifidx   = WIFI_IF_STA;   // required on Espressif core 3.x

    if (esp_now_add_peer(&peer) != ESP_OK) {
        Serial.println("[ESP-NOW] Failed to add Nano as peer");
        return false;
    }

    Serial.println("[ESP-NOW] Initialised, Nano added as peer");
    return true;
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("\n=== Helmet BLE scanner ===");

    if (!espNowInit()) {
        Serial.println("ESP-NOW failed — halting");
        while (true) delay(100);
    }

    NimBLEDevice::init("");
    pScan = NimBLEDevice::getScan();
    pScan->setScanCallbacks(&scanCallbacks, true);
    pScan->setActiveScan(false);
    pScan->setInterval(160);
    pScan->setWindow(160);
    pScan->start(0, false);
    Serial.println("Scanning...");
}

void loop() {
    delay(1000);
}