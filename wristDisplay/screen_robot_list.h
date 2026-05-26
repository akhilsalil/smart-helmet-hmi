#ifndef SCREEN_ROBOT_LIST_H
#define SCREEN_ROBOT_LIST_H

#include <lvgl.h>

// Builds the robot list screen.
// Fetches all robots from mock API via WiFi.
// Shows scan button → triggers helmet ESP-NOW scan.
// On robot selected → populates currentRobot, switchTo(SCREEN_ROBOT)
void createRobotListScreen();

// Call this when ESP-NOW scan result arrives from helmet.
// Populates currentRobot and switches to SCREEN_ROBOT.
void onScanResultReceived(int robotId);

#endif // SCREEN_ROBOT_LIST_H
