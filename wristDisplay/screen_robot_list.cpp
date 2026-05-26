#include "screen_robot_list.h"
#include "screen_manager.h"
#include "robot_data.h"
#include "wifi_manager.h"
#include "config.h"

#define COL_BG        0x0d1117
#define COL_PANEL     0x161b22
#define COL_ACCENT    0x00d4ff
#define COL_TEXT      0xe6edf3
#define COL_SUBTEXT   0x8b949e
#define COL_SAFE      0x00ff88
#define COL_CAUTION   0xffd700
#define COL_DANGER    0xff3333
#define COL_SCAN_BTN  0x1f6feb

extern RobotData robotList[];
extern int       robotListCount;
extern RobotData currentRobot;

static void onRobotCardTapped(lv_event_t *e) {
    int index = (int)(intptr_t)lv_event_get_user_data(e);
    if (index >= 0 && index < robotListCount) {
        currentRobot = robotList[index];
        switchTo(SCREEN_ROBOT);
    }
}

static void onScanPressed(lv_event_t *e) {
    Serial.println("[Scan] pressed — ESP-NOW not yet implemented");
}

void createRobotListScreen() {
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(COL_BG), 0);

    // Header
    lv_obj_t *header = lv_obj_create(scr);
    lv_obj_set_size(header, SCREEN_WIDTH, 44);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(COL_PANEL), 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "SMART HELMET HMI");
    lv_obj_set_style_text_color(title, lv_color_hex(COL_ACCENT), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 12, 0);
    lv_obj_t *sub = lv_label_create(header);
    lv_label_set_text(sub, "Robots in Sector");
    lv_obj_set_style_text_color(sub, lv_color_hex(COL_SUBTEXT), 0);
    lv_obj_align(sub, LV_ALIGN_RIGHT_MID, -12, 0);
    lv_obj_t *line = lv_obj_create(scr);
    lv_obj_set_size(line, SCREEN_WIDTH, 2);
    lv_obj_set_pos(line, 0, 44);
    lv_obj_set_style_bg_color(line, lv_color_hex(COL_ACCENT), 0);
    lv_obj_set_style_border_width(line, 0, 0);
    lv_obj_set_style_radius(line, 0, 0);

    // Scan button (stub — needs ESP-NOW)
    lv_obj_t *scanBtn = lv_btn_create(scr);
    lv_obj_set_size(scanBtn, 120, 36);
    lv_obj_set_pos(scanBtn, SCREEN_WIDTH - 130, 52);
    lv_obj_set_style_bg_color(scanBtn, lv_color_hex(COL_SCAN_BTN), 0);
    lv_obj_set_style_radius(scanBtn, 8, 0);
    lv_obj_set_style_border_width(scanBtn, 0, 0);
    lv_obj_set_style_shadow_width(scanBtn, 0, 0);
    lv_obj_add_event_cb(scanBtn, onScanPressed, LV_EVENT_CLICKED, NULL);
    lv_obj_t *scanLabel = lv_label_create(scanBtn);
    lv_label_set_text(scanLabel, LV_SYMBOL_EYE_OPEN " SCAN");
    lv_obj_set_style_text_color(scanLabel, lv_color_hex(0xffffff), 0);
    lv_obj_center(scanLabel);

    // Fetch robot list
    char response[1024];
    char url[64];
    snprintf(url, sizeof(url), API_BASE_URL "/robots");
    int statusCode = httpGet(url, response, sizeof(response));

    if (statusCode != 200) {
        lv_obj_t *errLabel = lv_label_create(scr);
        char errBuf[64];
        snprintf(errBuf, sizeof(errBuf), "Failed to fetch robots\nHTTP %d", statusCode);
        lv_label_set_text(errLabel, errBuf);
        lv_obj_set_style_text_color(errLabel, lv_color_hex(COL_DANGER), 0);
        lv_obj_set_style_text_align(errLabel, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(errLabel, LV_ALIGN_CENTER, 0, 20);
        return;
    }

    robotListCount = parseRobotsJson(response, robotList, MAX_ROBOTS);

    if (robotListCount == 0) {
        lv_obj_t *emptyLabel = lv_label_create(scr);
        lv_label_set_text(emptyLabel, "No robots found in sector");
        lv_obj_set_style_text_color(emptyLabel, lv_color_hex(COL_SUBTEXT), 0);
        lv_obj_align(emptyLabel, LV_ALIGN_CENTER, 0, 20);
        return;
    }

    // Cards
    int cardW = 330, cardH = 56, cardGap = 6, cardX = 8, cardY = 52;
    for (int i = 0; i < robotListCount; i++) {
        RobotData &r = robotList[i];
        lv_obj_t *card = lv_obj_create(scr);
        lv_obj_set_size(card, cardW, cardH);
        lv_obj_set_pos(card, cardX, cardY + i * (cardH + cardGap));
        lv_obj_set_style_bg_color(card, lv_color_hex(COL_PANEL), 0);
        lv_obj_set_style_border_color(card, lv_color_hex(0x30363d), 0);
        lv_obj_set_style_border_width(card, 1, 0);
        lv_obj_set_style_radius(card, 8, 0);
        lv_obj_set_style_pad_all(card, 8, 0);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_bg_color(card, lv_color_hex(0x21262d), LV_STATE_PRESSED);

        lv_obj_t *nameLabel = lv_label_create(card);
        lv_label_set_text(nameLabel, r.name);
        lv_obj_set_style_text_color(nameLabel, lv_color_hex(COL_ACCENT), 0);
        lv_obj_set_pos(nameLabel, 0, 0);

        lv_obj_t *typeLabel = lv_label_create(card);
        lv_label_set_text(typeLabel, r.type);
        lv_obj_set_style_text_color(typeLabel, lv_color_hex(COL_SUBTEXT), 0);
        lv_obj_set_pos(typeLabel, 0, 20);

        lv_obj_t *statusLabel = lv_label_create(card);
        lv_label_set_text(statusLabel, r.status[0] ? r.status : "UNKNOWN");
        lv_obj_set_style_text_color(statusLabel, lv_color_hex(getStatusColor(r.status)), 0);
        lv_obj_align(statusLabel, LV_ALIGN_RIGHT_MID, 0, 0);

        lv_obj_add_event_cb(card, onRobotCardTapped, LV_EVENT_CLICKED, (void*)(intptr_t)i);
    }
}

void onScanResultReceived(int robotId) {
    for (int i = 0; i < robotListCount; i++) {
        if (robotList[i].id == robotId) {
            currentRobot = robotList[i];
            switchTo(SCREEN_ROBOT);
            return;
        }
    }
    Serial.printf("[Scan] Robot ID %d not found in list\n", robotId);
}