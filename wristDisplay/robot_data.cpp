#include "robot_data.h"
#include <string.h>
#include <Arduino.h>

// ---------------------------------------------------------------------------
// Capability → command button mapping
// ---------------------------------------------------------------------------
int getCommandButtons(const RobotData &robot, CommandButton buttons[], int maxButtons) {
    int count = 0;
    for (int i = 0; i < robot.capabilityCount && count < maxButtons; i++) {
        const char* cap = robot.capabilities[i];
        if      (strcasecmp(cap, "forward") == 0)                               { strncpy(buttons[count].label, "FORWARD", 16); strncpy(buttons[count].command, "forward", 16); count++; }
        else if (strcasecmp(cap, "reverse") == 0)                               { strncpy(buttons[count].label, "REVERSE", 16); strncpy(buttons[count].command, "reverse", 16); count++; }
        else if (strcasecmp(cap, "turn left") == 0)                             { strncpy(buttons[count].label, "LEFT",    16); strncpy(buttons[count].command, "left",    16); count++; }
        else if (strcasecmp(cap, "turn right") == 0)                            { strncpy(buttons[count].label, "RIGHT",   16); strncpy(buttons[count].command, "right",   16); count++; }
        else if (strcasecmp(cap, "stop") == 0 || strcasecmp(cap, "stop on command") == 0) { strncpy(buttons[count].label, "STOP", 16); strncpy(buttons[count].command, "stop", 16); count++; }
        else if (strcasecmp(cap, "pick_object") == 0)                                  { strncpy(buttons[count].label, "PICK", 16); strncpy(buttons[count].command, "pick_object", 16); count++; }
        // Autonomous caps (line following, obstacle avoidance etc.) → no button
    }
    return count;
}

// ---------------------------------------------------------------------------
// Maps status string → display colour
// Hardcoded hex values (colour constants live in screen files for now)
// Used by screen_robot_list.cpp and screen_robot.cpp
// ---------------------------------------------------------------------------
uint32_t getStatusColor(const char* status) {
    if (strcasecmp(status, "active") == 0)    return 0x00ff88;  // green
    if (strcasecmp(status, "idle") == 0)      return 0x8b949e;  // grey
    if (strcasecmp(status, "commanded") == 0) return 0xffd700;  // yellow
    return 0x8b949e;  // fallback grey for unknown status
}

// ---------------------------------------------------------------------------
// Minimal JSON parser helpers
// Arduino has no stdlib JSON — we parse manually using string search.
// This is not a general JSON parser — it only handles the exact shape of
// our robots.json / tasks.json responses.
// ---------------------------------------------------------------------------

// Extract a string value for a given key from a flat JSON object string.
// e.g. extractString(json, "name", buf, 64) → fills buf with "Line Follower Bot A"
static bool extractString(const char* json, const char* key, char* buf, size_t bufSize) {
    // Build search pattern: "key":"
    char pattern[64];
    snprintf(pattern, sizeof(pattern), "\"%s\":\"", key);
    const char* found = strstr(json, pattern);
    if (!found) return false;
    found += strlen(pattern);
    const char* end = strchr(found, '"');
    if (!end) return false;
    size_t len = min((size_t)(end - found), bufSize - 1);
    strncpy(buf, found, len);
    buf[len] = '\0';
    return true;
}

// Extract an integer value for a given key from a flat JSON object string.
// e.g. extractInt(json, "battery", val) → val = 78
static bool extractInt(const char* json, const char* key, int &val) {
    char pattern[64];
    snprintf(pattern, sizeof(pattern), "\"%s\":", key);
    const char* found = strstr(json, pattern);
    if (!found) return false;
    found += strlen(pattern);
    val = atoi(found);
    return true;
}

// Parse capabilities array from JSON string.
// Looks for "capabilities":["a","b","c"] and fills robot.capabilities[]
static void extractCapabilities(const char* json, RobotData &robot) {
    robot.capabilityCount = 0;
    const char* arrStart = strstr(json, "\"capabilities\":[");
    if (!arrStart) return;
    arrStart = strchr(arrStart, '[');
    if (!arrStart) return;
    arrStart++; // skip '['

    const char* p = arrStart;
    while (*p && *p != ']' && robot.capabilityCount < MAX_CAPABILITIES) {
        // Find opening quote
        const char* q = strchr(p, '"');
        if (!q || *q == ']') break;
        q++; // skip opening quote
        const char* end = strchr(q, '"');
        if (!end) break;
        size_t len = min((size_t)(end - q), (size_t)31);
        strncpy(robot.capabilities[robot.capabilityCount], q, len);
        robot.capabilities[robot.capabilityCount][len] = '\0';
        robot.capabilityCount++;
        p = end + 1;
    }
}

// ---------------------------------------------------------------------------
// Parse a single robot JSON object (from /robot/<id> or inside /robots array)
// ---------------------------------------------------------------------------
bool parseRobotJson(const char* json, RobotData &robot) {
    memset(&robot, 0, sizeof(RobotData));
    extractInt(json,    "id",           robot.id);
    extractString(json, "name",         robot.name,         sizeof(robot.name));
    extractString(json, "type",         robot.type,         sizeof(robot.type));
    extractString(json, "current_task", robot.currentTask,  sizeof(robot.currentTask));
    extractString(json, "status",       robot.status,       sizeof(robot.status));
    extractInt(json,    "battery",      robot.battery);
    extractInt(json,    "completion",   robot.completion);
    extractInt(json,    "danger_level", robot.dangerLevel);
    extractString(json, "danger_message", robot.dangerMessage, sizeof(robot.dangerMessage));
    extractCapabilities(json, robot);
    return robot.id > 0;
}

// ---------------------------------------------------------------------------
// Parse /robots response — JSON array of robot objects
// Returns how many robots were parsed
// ---------------------------------------------------------------------------
int parseRobotsJson(const char* json, RobotData robots[], int maxRobots) {
    int count = 0;
    const char* p = json;

    while (count < maxRobots) {
        // Find start of next object
        const char* objStart = strchr(p, '{');
        if (!objStart) break;

        // Find matching closing brace
        int depth = 0;
        const char* objEnd = objStart;
        while (*objEnd) {
            if (*objEnd == '{') depth++;
            else if (*objEnd == '}') { depth--; if (depth == 0) break; }
            objEnd++;
        }
        if (*objEnd != '}') break;

        // Copy object into temp buffer and parse
        size_t objLen = objEnd - objStart + 1;
        char objBuf[512];
        if (objLen < sizeof(objBuf)) {
            strncpy(objBuf, objStart, objLen);
            objBuf[objLen] = '\0';
            if (parseRobotJson(objBuf, robots[count])) {
                count++;
            }
        }
        p = objEnd + 1;
    }
    return count;
}

// ---------------------------------------------------------------------------
// Mock data — used when API not available yet
// ---------------------------------------------------------------------------
void loadMockRobotData(RobotData &robot) {
    robot.id          = 1;
    robot.battery     = 78;
    robot.completion  = 45;
    robot.dangerLevel = 0;
    strncpy(robot.name,          "Line Follower Bot A", 64);
    strncpy(robot.type,          "AMR",                 32);
    strncpy(robot.currentTask,   "Following path B3",   64);
    strncpy(robot.status,        "ACTIVE",              16);
    strncpy(robot.dangerMessage, "",                    64);
    robot.capabilityCount = 3;
    strncpy(robot.capabilities[0], "line following",     32);
    strncpy(robot.capabilities[1], "obstacle avoidance", 32);
    strncpy(robot.capabilities[2], "stop on command",    32);
}