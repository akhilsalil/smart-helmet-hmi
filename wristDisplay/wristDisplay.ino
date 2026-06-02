#include <lvgl.h>
#include <TFT_eSPI.h>

#include "config.h"
#include "display_init.h"
#include "touch_init.h"
#include "robot_data.h"
#include "wifi_manager.h"
#include "screen_manager.h"
#include "espnow_manager.h"

// TFT instance — declared here, extern'd in display_init.cpp
TFT_eSPI tft = TFT_eSPI();

// Global current robot — populated when worker selects or scans a robot
RobotData currentRobot;

// Global robot list — populated on robot list screen load
RobotData robotList[MAX_ROBOTS];
int robotListCount = 0;

void setup() {
    Serial.begin(115200);
    delay(500);

    lv_init();
    initDisplay();
    initTouch();

    // Connect to WiFi before any screen loads
    // Shows nothing on screen during this — happens fast (<2s on good hotspot)
    Serial.println("[Setup] Connecting to WiFi...");
    if (wifiConnect(WIFI_SSID, WIFI_PASSWORD)) {
        Serial.println("[Setup] WiFi connected");
    } else {
        Serial.println("[Setup] WiFi failed — continuing anyway");
    }

    if (!espNowInit()) {
    Serial.println("ESP-NOW init failed");
    // Decide what to do. For now, continue — wrist still works for HTTP commands.
    // SCAN button will just print "send failed" when tapped.
}

    // Boot into PIN screen
    switchTo(SCREEN_PIN);
}

void loop() {
    lv_timer_handler();
    espNowPoll();
    delay(LVGL_TICK_MS);
}
