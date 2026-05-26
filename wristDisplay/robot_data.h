#ifndef ROBOT_DATA_H
#define ROBOT_DATA_H

#include <Arduino.h>
#include "config.h"

// --- Max capabilities a robot can have ---
#define MAX_CAPABILITIES 8

// --- Struct holding all robot info ---
// Populated either from mock data or real API response
struct RobotData {
    int     id;
    char    name[64];
    char    type[32];
    char    currentTask[64];
    char    status[16];
    int     battery;
    int     completion;
    int     dangerLevel;
    char    dangerMessage[64];
    char    capabilities[MAX_CAPABILITIES][32];
    int     capabilityCount;
};

// --- Command button descriptor ---
struct CommandButton {
    char label[16];
    char command[16];
};

// --- Maps robot capabilities → command buttons to show ---
// Returns number of buttons populated
int getCommandButtons(const RobotData &robot, CommandButton buttons[], int maxButtons);

// --- Maps robot status string → display colour (hex) ---
// "active" → green, "idle" → grey, "commanded" → yellow, fallback → grey
uint32_t getStatusColor(const char* status);

// --- Parse a single robot JSON object string into a RobotData struct ---
// Used when parsing /robot/<id> or individual entries from /robots
bool parseRobotJson(const char* json, RobotData &robot);

// --- Parse /robots response (array of robots) into an array of RobotData ---
// Returns number of robots parsed
int parseRobotsJson(const char* json, RobotData robots[], int maxRobots);

// --- Mock data loader — used during development without API ---
void loadMockRobotData(RobotData &robot);

#endif // ROBOT_DATA_H