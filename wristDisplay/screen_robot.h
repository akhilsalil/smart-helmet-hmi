#ifndef SCREEN_ROBOT_H
#define SCREEN_ROBOT_H

#include <lvgl.h>
#include "robot_data.h"

// Builds and loads the robot info + commands screen.
// Pass in a populated RobotData struct.
void createRobotScreen(const RobotData &robot);

#endif // SCREEN_ROBOT_H
