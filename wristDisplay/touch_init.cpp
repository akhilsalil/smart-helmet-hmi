#include "touch_init.h"
#include "config.h"

static GT911_Lite tp;
static TwoWire wire(0);

void my_touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {
    if (tp.read() && tp.touches > 0) {
        data->state = LV_INDEV_STATE_PR;
        // GT911 reports in portrait (320x480).
        // Rotate to landscape (480x320): swap axes, mirror Y
        uint16_t raw_x = tp.points[0].x;
        uint16_t raw_y = tp.points[0].y;
        data->point.x = raw_y;
        data->point.y = SCREEN_HEIGHT - raw_x;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

void initTouch() {
    wire.begin(TOUCH_SDA, TOUCH_SCL);
    tp.begin(&wire);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type    = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);
}