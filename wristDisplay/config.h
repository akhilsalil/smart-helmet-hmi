#ifndef CONFIG_H
#define CONFIG_H
 
// --- Screen dimensions ---
#define SCREEN_WIDTH  480
#define SCREEN_HEIGHT 320
 
// --- Touch I2C pins (GT911) ---
#define TOUCH_SDA 33
#define TOUCH_SCL 32
 
// --- Danger levels ---
#define DANGER_SAFE      0
#define DANGER_CAUTION   1
#define DANGER_SENSITIVE 2
#define DANGER_HIGH      3
 
// --- LVGL tick interval (ms) ---
#define LVGL_TICK_MS 5
 
// --- Worker PIN (hardcoded for prototype) ---
#define WORKER_PIN "1234"
 
// --- WiFi credentials (never commit this file) ---
#define WIFI_SSID     "9ECE"
#define WIFI_PASSWORD "wifidepwd"
 
// --- Mock API base URL ---
#define API_BASE_URL  "http://192.168.2.102:5000/"
 
// --- Max robots the display can hold in memory ---
#define MAX_ROBOTS 10
 
#endif // CONFIG_H