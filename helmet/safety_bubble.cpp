#include "safety_bubble.h"
#include "espnow_manager.h"

// -----------------------------------------------------------------------------
// Tunable constants — adjust empirically once we have real RF measurements
// -----------------------------------------------------------------------------
static const int8_t   RSSI_ENTER_THRESHOLD   = -70;   // closer than this → alarm
static const int8_t   RSSI_EXIT_THRESHOLD    = -78;   // farther than this → clear
static const uint32_t BEACON_TIMEOUT_MS      = 3000;  // robot considered gone after this
static const uint32_t MIN_ALARM_HOLD_MS      = 2000;  // alarm stays on at least this long
static const uint8_t  DANGER_THRESHOLD       = 1;     // CAUTION or higher triggers alarm
static const uint8_t  TRACKED_ROBOT_ID       = 2;     // Amazon bot — only one we track for now

// EMA smoothing factor. Higher = more responsive but noisier.
// 0.3 means each new reading contributes 30%, smoothed value keeps 70% of old.
static const float EMA_ALPHA = 0.3f;

// -----------------------------------------------------------------------------
// State — only tracking one robot for now
// -----------------------------------------------------------------------------
static bool     haveData       = false;   // have we ever received a beacon
static float    smoothedRssi   = -100.0f; // EMA-smoothed RSSI
static uint8_t  lastDanger     = 0;
static uint32_t lastBeaconAt   = 0;

static bool     inAlarm        = false;
static uint32_t alarmEnteredAt = 0;

void safetyBubbleInit() {
    haveData       = false;
    smoothedRssi   = -100.0f;
    lastDanger     = 0;
    inAlarm        = false;
}

void safetyBubbleOnBeacon(uint8_t robotId, uint8_t dangerLevel, int8_t rssi) {
    // Only track our one robot for now
    if (robotId != TRACKED_ROBOT_ID) return;

    // First-ever beacon: seed the EMA so it doesn't slowly ramp up from -100
    if (!haveData) {
        smoothedRssi = (float)rssi;
        haveData     = true;
    } else {
        smoothedRssi = EMA_ALPHA * (float)rssi + (1.0f - EMA_ALPHA) * smoothedRssi;
    }

    lastDanger   = dangerLevel;
    lastBeaconAt = millis();
}

void safetyBubbleUpdate() {
    // No data yet — nothing to evaluate
    if (!haveData) return;

    // Beacon timeout — robot considered gone
    uint32_t sinceLastBeacon = millis() - lastBeaconAt;
    if (sinceLastBeacon > BEACON_TIMEOUT_MS) {
        if (inAlarm && (millis() - alarmEnteredAt >= MIN_ALARM_HOLD_MS)) {
            // Robot gone AND we've held the alarm long enough → clear
            inAlarm = false;
            espNowSendAlarmState(false, TRACKED_ROBOT_ID, lastDanger);
            Serial.println("[Bubble] Robot timed out, alarm cleared");
        }
        return;
    }

    // Alarm conditions: dangerous robot + close enough
    bool dangerous = (lastDanger >= DANGER_THRESHOLD);

    if (!inAlarm) {
        if (dangerous && smoothedRssi >= RSSI_ENTER_THRESHOLD) {
            inAlarm        = true;
            alarmEnteredAt = millis();
            Serial.println("!!!!! ABOUT TO CALL ESPNOW SEND !!!!!");
            espNowSendAlarmState(true, TRACKED_ROBOT_ID, lastDanger);
            Serial.printf("[Bubble] ALARM ENTERED — robot %d, danger=%d, rssi=%.1f\n",
                          TRACKED_ROBOT_ID, lastDanger, smoothedRssi);
        }
    } else {
        // Currently in alarm — should we exit?
        // Conditions to exit: held long enough AND (far away OR no longer dangerous)
        bool heldLongEnough = (millis() - alarmEnteredAt >= MIN_ALARM_HOLD_MS);
        bool farEnough      = (smoothedRssi <= RSSI_EXIT_THRESHOLD);
        bool noLongerDanger = (!dangerous);
        if (heldLongEnough && (farEnough || noLongerDanger)) {
            inAlarm = false;
            espNowSendAlarmState(false, TRACKED_ROBOT_ID, lastDanger);
            Serial.printf("[Bubble] Alarm cleared — rssi=%.1f danger=%d\n",
                          smoothedRssi, lastDanger);
        }
    }
}