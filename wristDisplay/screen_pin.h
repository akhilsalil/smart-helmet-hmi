#ifndef SCREEN_PIN_H
#define SCREEN_PIN_H

#include <lvgl.h>

// Builds the PIN entry screen.
// On correct PIN → calls switchTo(SCREEN_ROBOT_LIST)
void createPinScreen();

#endif // SCREEN_PIN_H
