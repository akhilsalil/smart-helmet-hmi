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

// Beacon broadcast by robots (planned). For now, mock-generated via Serial.
// Sent on a periodic timer; helmet listens passively.
typedef struct {
    uint8_t msgType;      // 10 = robot_beacon
    uint8_t robotId;
    uint8_t dangerLevel;  // 0=safe, 1=caution, 2=sensitive, 3=high
    int8_t  rssiPad;      // unused in struct, RSSI comes from receive metadata
} __attribute__((packed)) RobotBeacon;


// Sent by the BLE scanner board to the Nano.
// RSSI is measured at the scanner, packed into the payload here.
typedef struct {
    uint8_t msgType;      // 20 = scanner beacon
    uint8_t robotId;
    uint8_t dangerLevel;
    int8_t  rssi;
} __attribute__((packed)) ScannerBeacon;

// Sent helmet → wrist when the safety bubble alarm enters or clears.
typedef struct {
    uint8_t msgType;      // 30 = alarm state
    uint8_t alarmActive;  // 1 = entering, 0 = clearing
    uint8_t robotId;
    uint8_t dangerLevel;
} __attribute__((packed)) AlarmStateMsg;


#define ESPNOW_MSG_ALARM_STATE 30
#define ESPNOW_MSG_SCANNER_BEACON 20
#define ESPNOW_MSG_ROBOT_BEACON  10
#define ESPNOW_MSG_SCAN_REQUEST  1
#define ESPNOW_MSG_SCAN_RESULT   2
#define ESPNOW_MSG_SCAN_FAILED   3

bool espNowInit();

bool espNowSendToWrist(const EspNowMessage &msg);
bool espNowSendAlarmState(bool active, uint8_t robotId, uint8_t dangerLevel);

// Called from loop() — processes any pending scan request after stub delay.
// Replace with real HuskyLens-driven scan when hardware is wired.
void espNowProcessPendingScan();

// Called from loop() — feeds pending scanner beacons into the safety bubble.
void espNowProcessPendingBeacon();

#endif