#ifndef TOUCH_INIT_H
#define TOUCH_INIT_H

#include <lvgl.h>
#include "gt911_lite.h"
#include "Wire.h"
#include "config.h"

// Call once in setup() — initialises GT911 and registers LVGL input driver
void initTouch();

// LVGL touch read callback — do not call directly
void my_touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data);

#endif // TOUCH_INIT_H
