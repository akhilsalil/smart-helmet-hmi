#include "screen_manager.h"
#include "screen_pin.h"
#include "screen_robot_list.h"
#include "screen_robot.h"
#include "robot_data.h"
#include <lvgl.h>

static ScreenID _currentScreen = SCREEN_PIN;

ScreenID currentScreen() {
    return _currentScreen;
}

void switchTo(ScreenID screen) {
    // Clean up current LVGL screen
    lv_obj_clean(lv_scr_act());

    _currentScreen = screen;

    switch (screen) {
        case SCREEN_PIN:
            createPinScreen();
            break;

        case SCREEN_ROBOT_LIST:
            createRobotListScreen();
            break;

        case SCREEN_ROBOT:
            // Caller must have populated currentRobot before switching
            extern RobotData currentRobot;
            createRobotScreen(currentRobot);
            break;
    }
}
