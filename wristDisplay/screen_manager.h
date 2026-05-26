#ifndef SCREEN_MANAGER_H
#define SCREEN_MANAGER_H

// --- All screens in the app ---
typedef enum {
    SCREEN_PIN,         // Worker PIN entry
    SCREEN_ROBOT_LIST,  // All robots in sector
    SCREEN_ROBOT,       // Robot detail + commands
} ScreenID;

// Switch to a screen by ID.
// Cleans up the current screen and builds the new one.
void switchTo(ScreenID screen);

// Which screen is currently active
ScreenID currentScreen();

#endif // SCREEN_MANAGER_H
