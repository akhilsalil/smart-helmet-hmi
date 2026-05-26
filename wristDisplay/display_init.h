#ifndef DISPLAY_INIT_H
#define DISPLAY_INIT_H

#include <lvgl.h>
#include <TFT_eSPI.h>
#include "config.h"

// Call once in setup() — initialises TFT, LVGL draw buffer, and registers display driver
void initDisplay();

// LVGL flush callback — do not call directly
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);

#endif // DISPLAY_INIT_H
