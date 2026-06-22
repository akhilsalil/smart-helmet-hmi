#include "screen_robot.h"
#include "screen_manager.h"
#include "wifi_manager.h"
#include "config.h"
#include <stdio.h>
#include <string.h>
#include "auth.h"

// currentRobot is defined in the main .ino, externed here so the command
// handler can read robot.id at click time
extern RobotData currentRobot;

// File-static pointer to the error label so the command handler can update it.
// Created in createRobotScreen, hidden by default, populated/shown on failure.
// CAVEAT: this is screen-scoped. If a callback ever fires after switching screens,
// it would touch a dangling pointer. Same shape of bug as the PIN screen issue —
// currently safe because all our callbacks (LV_EVENT_CLICKED on buttons) fire
// synchronously from user input on the active screen, not from deferred timers.
static lv_obj_t *err_label = NULL;
static lv_timer_t *cmd_feedback_timer = NULL;

static void onBackPressed(lv_event_t *e) {
    if (cmd_feedback_timer) {
        lv_timer_del(cmd_feedback_timer);
        cmd_feedback_timer = NULL;
    }
    err_label = NULL;  // null before screen destruction to prevent stale ref
    switchTo(SCREEN_ROBOT_LIST);
}

// --- Colour palette ---
#define COL_BG          0x0d1117
#define COL_PANEL       0x161b22
#define COL_ACCENT      0x00d4ff
#define COL_TEXT        0xe6edf3
#define COL_SAFE        0x00ff88
#define COL_CAUTION     0xffd700
#define COL_SENSITIVE   0xff8800
#define COL_DANGER      0xff3333
#define COL_BTN_FWD     0x238636
#define COL_BTN_REV     0x238636
#define COL_BTN_LEFT    0x1f6feb
#define COL_BTN_RIGHT   0x1f6feb
#define COL_BTN_STOP    0xda3633
#define COL_BTN_PICK    0x9c27b0

static uint32_t getDangerColor(int level) {
    switch (level) {
        case DANGER_SAFE:      return COL_SAFE;
        case DANGER_CAUTION:   return COL_CAUTION;
        case DANGER_SENSITIVE: return COL_SENSITIVE;
        case DANGER_HIGH:      return COL_DANGER;
        default:               return COL_SAFE;
    }
}

static const char* getDangerText(int level) {
    switch (level) {
        case DANGER_SAFE:      return LV_SYMBOL_OK " SAFE";
        case DANGER_CAUTION:   return LV_SYMBOL_WARNING " CAUTION";
        case DANGER_SENSITIVE: return LV_SYMBOL_WARNING " SENSITIVE";
        case DANGER_HIGH:      return LV_SYMBOL_CLOSE " DANGER";
        default:               return LV_SYMBOL_OK " SAFE";
    }
}

static uint32_t getButtonColor(const char* command) {
    if (strcmp(command, "forward") == 0) return COL_BTN_FWD;
    if (strcmp(command, "reverse") == 0) return COL_BTN_REV;
    if (strcmp(command, "left")    == 0) return COL_BTN_LEFT;
    if (strcmp(command, "right")   == 0) return COL_BTN_RIGHT;
    if (strcmp(command, "stop")    == 0) return COL_BTN_STOP;
    if (strcmp(command, "pick_object") == 0) return COL_BTN_PICK;
    return 0x444444;
}

// ---------------------------------------------------------------------------
// sendCommand — POST {"command":"...","operator":"helmet-display"}
//   to /robot/<robotId>/command
// Returns true on HTTP 2xx, false otherwise.
// On failure, fills errMsg with a short human-readable reason.
// ---------------------------------------------------------------------------
static bool sendCommand(int robotId, const char* command, char* errMsg, size_t errMsgSize) {
    char url[80];
    snprintf(url, sizeof(url), "%s/robot/%d/command", API_BASE_URL, robotId);

    char body[80];
    snprintf(body, sizeof(body), "{\"command\":\"%s\",\"operator\":\"helmet-display\"}", command);

    char response[128];  // response body — we don't use it but httpPost requires a buffer
    int statusCode = httpPost(url, body, response, sizeof(response));

    if (statusCode >= 200 && statusCode < 300) {
        errMsg[0] = '\0';
        return true;
    }

    // Map status to readable message
    if (statusCode == -1)        snprintf(errMsg, errMsgSize, "No WiFi");
    else if (statusCode < 0)     snprintf(errMsg, errMsgSize, "Network error");
    else if (statusCode == 503)  snprintf(errMsg, errMsgSize, "Robot offline");
    else if (statusCode == 504)  snprintf(errMsg, errMsgSize, "Robot timeout");
    else                         snprintf(errMsg, errMsgSize, "Failed (HTTP %d)", statusCode);

    Serial.printf("[Cmd] %s on robot %d failed: %s\n", command, robotId, errMsg);
    return false;
}

static void hideCommandFeedback(lv_timer_t *t) {
    if (err_label) {
        lv_obj_add_flag(err_label, LV_OBJ_FLAG_HIDDEN);
    }
    cmd_feedback_timer = NULL;
}

// Click handler for command buttons. Command string passed via user_data.
static void onCommandPressed(lv_event_t *e) {
    const char* command = (const char*)lv_event_get_user_data(e);
    if (!command) return;

    char errMsg[48];
    bool ok = sendCommand(currentRobot.id, command, errMsg, sizeof(errMsg));

    if (!err_label) return;

    if (cmd_feedback_timer) {
        lv_timer_del(cmd_feedback_timer);
        cmd_feedback_timer = NULL;
    }

    if (ok) {
        lv_label_set_text(err_label, "Sent " LV_SYMBOL_OK);
        lv_obj_set_style_text_color(err_label, lv_color_hex(COL_SAFE), 0);
        lv_obj_clear_flag(err_label, LV_OBJ_FLAG_HIDDEN);
        cmd_feedback_timer = lv_timer_create(hideCommandFeedback, 1200, NULL);
        lv_timer_set_repeat_count(cmd_feedback_timer, 1);
    } else {
        lv_label_set_text(err_label, errMsg);
        lv_obj_set_style_text_color(err_label, lv_color_hex(COL_DANGER), 0);
        lv_obj_clear_flag(err_label, LV_OBJ_FLAG_HIDDEN);
    }
}

void createRobotScreen(const RobotData &robot) {
    err_label = NULL;  // reset on every screen build
    cmd_feedback_timer = NULL;

    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(COL_BG), 0);

    // =========================================================
    // HEADER BAR
    // =========================================================
    lv_obj_t *header = lv_obj_create(scr);
    lv_obj_set_size(header, SCREEN_WIDTH, 44);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(COL_PANEL), 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_set_style_pad_all(header, 0, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(header);
    const char* roleStr = (currentRole == ROLE_OPERATOR) ? "OPERATOR" : "VIEWER";
    char titleBuf[48];
    snprintf(titleBuf, sizeof(titleBuf), "SMART HELMET HMI - %s", roleStr);
    lv_label_set_text(title, titleBuf);
    lv_obj_set_style_text_color(title, lv_color_hex(COL_ACCENT), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 12, 0);

    lv_obj_t *danger_label = lv_label_create(header);
    lv_label_set_text(danger_label, getDangerText(robot.dangerLevel));
    lv_obj_set_style_text_color(danger_label, lv_color_hex(getDangerColor(robot.dangerLevel)), 0);
    lv_obj_align(danger_label, LV_ALIGN_RIGHT_MID, -12, 0);

    // Accent line under header
    lv_obj_t *header_line = lv_obj_create(scr);
    lv_obj_set_size(header_line, SCREEN_WIDTH, 2);
    lv_obj_set_pos(header_line, 0, 44);
    lv_obj_set_style_bg_color(header_line, lv_color_hex(COL_ACCENT), 0);
    lv_obj_set_style_border_width(header_line, 0, 0);
    lv_obj_set_style_radius(header_line, 0, 0);

    // =========================================================
    // INFO PANEL (left)
    // =========================================================
    lv_obj_t *info_panel = lv_obj_create(scr);
    lv_obj_set_size(info_panel, 290, 262);
    lv_obj_set_pos(info_panel, 8, 50);
    lv_obj_set_style_bg_color(info_panel, lv_color_hex(COL_PANEL), 0);
    lv_obj_set_style_border_color(info_panel, lv_color_hex(COL_ACCENT), 0);
    lv_obj_set_style_border_width(info_panel, 1, 0);
    lv_obj_set_style_radius(info_panel, 8, 0);
    lv_obj_set_style_pad_all(info_panel, 12, 0);
    lv_obj_clear_flag(info_panel, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *name_label = lv_label_create(info_panel);
    lv_label_set_text(name_label, robot.name);
    lv_obj_set_style_text_color(name_label, lv_color_hex(COL_ACCENT), 0);
    lv_obj_set_style_text_font(name_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(name_label, 0, 0);

    lv_obj_t *type_label = lv_label_create(info_panel);
    char type_buf[48];
    snprintf(type_buf, sizeof(type_buf), "Type:    %s", robot.type);
    lv_label_set_text(type_label, type_buf);
    lv_obj_set_style_text_color(type_label, lv_color_hex(COL_TEXT), 0);
    lv_obj_set_pos(type_label, 0, 28);

    lv_obj_t *task_label = lv_label_create(info_panel);
    char task_buf[80];
    snprintf(task_buf, sizeof(task_buf), "Task:    %s", robot.currentTask);
    lv_label_set_text(task_label, task_buf);
    lv_obj_set_style_text_color(task_label, lv_color_hex(COL_TEXT), 0);
    lv_obj_set_pos(task_label, 0, 52);

    lv_obj_t *status_label = lv_label_create(info_panel);
    char status_buf[32];
    snprintf(status_buf, sizeof(status_buf), "Status:  %s", robot.status);
    lv_label_set_text(status_label, status_buf);
    lv_obj_set_style_text_color(status_label, lv_color_hex(getStatusColor(robot.status)), 0);
    lv_obj_set_pos(status_label, 0, 76);

    lv_obj_t *battery_label = lv_label_create(info_panel);
    char battery_buf[32];
    snprintf(battery_buf, sizeof(battery_buf), "Battery: %d%%", robot.battery);
    lv_label_set_text(battery_label, battery_buf);
    lv_obj_set_style_text_color(battery_label, lv_color_hex(COL_TEXT), 0);
    lv_obj_set_pos(battery_label, 0, 100);

    lv_obj_t *bat_bar = lv_bar_create(info_panel);
    lv_obj_set_size(bat_bar, 220, 8);
    lv_obj_set_pos(bat_bar, 0, 118);
    lv_bar_set_value(bat_bar, robot.battery, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bat_bar, lv_color_hex(0x30363d), 0);
    lv_obj_set_style_bg_color(bat_bar, lv_color_hex(COL_SAFE), LV_PART_INDICATOR);

    lv_obj_t *completion_label = lv_label_create(info_panel);
    char completion_buf[32];
    snprintf(completion_buf, sizeof(completion_buf), "Done:    %d%%", robot.completion);
    lv_label_set_text(completion_label, completion_buf);
    lv_obj_set_style_text_color(completion_label, lv_color_hex(COL_TEXT), 0);
    lv_obj_set_pos(completion_label, 0, 136);

    lv_obj_t *comp_bar = lv_bar_create(info_panel);
    lv_obj_set_size(comp_bar, 220, 8);
    lv_obj_set_pos(comp_bar, 0, 154);
    lv_bar_set_value(comp_bar, robot.completion, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(comp_bar, lv_color_hex(0x30363d), 0);
    lv_obj_set_style_bg_color(comp_bar, lv_color_hex(COL_ACCENT), LV_PART_INDICATOR);

    // Back button — bottom of info panel
    lv_obj_t *backBtn = lv_btn_create(info_panel);
    lv_obj_set_size(backBtn, 100, 30);
    lv_obj_set_pos(backBtn, 0, 190);
    lv_obj_set_style_bg_color(backBtn, lv_color_hex(0x21262d), 0);
    lv_obj_set_style_bg_color(backBtn, lv_color_hex(0x30363d), LV_STATE_PRESSED);
    lv_obj_set_style_radius(backBtn, 6, 0);
    lv_obj_set_style_border_width(backBtn, 1, 0);
    lv_obj_set_style_border_color(backBtn, lv_color_hex(0x30363d), 0);
    lv_obj_set_style_shadow_width(backBtn, 0, 0);
    lv_obj_add_event_cb(backBtn, onBackPressed, LV_EVENT_CLICKED, NULL);

    lv_obj_t *backLabel = lv_label_create(backBtn);
    lv_label_set_text(backLabel, LV_SYMBOL_LEFT " Back");
    lv_obj_set_style_text_color(backLabel, lv_color_hex(0xe6edf3), 0);
    lv_obj_center(backLabel);

    // =========================================================
    // RIGHT PANEL — capabilities + commands
    // =========================================================
    lv_obj_t *btn_panel = lv_obj_create(scr);
    lv_obj_set_size(btn_panel, 168, 262);
    lv_obj_set_pos(btn_panel, 304, 50);
    lv_obj_set_style_bg_color(btn_panel, lv_color_hex(COL_PANEL), 0);
    lv_obj_set_style_border_color(btn_panel, lv_color_hex(COL_ACCENT), 0);
    lv_obj_set_style_border_width(btn_panel, 1, 0);
    lv_obj_set_style_radius(btn_panel, 8, 0);
    lv_obj_set_style_pad_all(btn_panel, 10, 0);
    lv_obj_clear_flag(btn_panel, LV_OBJ_FLAG_SCROLLABLE);

    // Get command buttons first — also determines whether to show capabilities section at all
    CommandButton cmdButtons[MAX_CAPABILITIES];
    int cmdCount = getCommandButtons(robot, cmdButtons, MAX_CAPABILITIES);

    // Hide capabilities section when every capability is also a command (redundant info).
    bool showCapabilities = (cmdCount < robot.capabilityCount);

    int capY = 20;
    if (showCapabilities) {
        // CAPABILITIES header
        lv_obj_t *cap_title = lv_label_create(btn_panel);
        lv_label_set_text(cap_title, "CAPABILITIES");
        lv_obj_set_style_text_color(cap_title, lv_color_hex(COL_ACCENT), 0);
        lv_obj_set_pos(cap_title, 0, 0);

        for (int i = 0; i < robot.capabilityCount; i++) {
            bool isCommandable = false;
            for (int j = 0; j < cmdCount; j++) {
                if (strcasestr(robot.capabilities[i], cmdButtons[j].command) != NULL) {
                    isCommandable = true;
                    break;
                }
            }
            if (strcasestr(robot.capabilities[i], "stop") != NULL && cmdCount > 0) isCommandable = true;

            lv_obj_t *cap_label = lv_label_create(btn_panel);
            char cap_buf[40];
            snprintf(cap_buf, sizeof(cap_buf), "%s %s",
                isCommandable ? LV_SYMBOL_PLAY : LV_SYMBOL_EYE_OPEN,
                robot.capabilities[i]);
            lv_label_set_text(cap_label, cap_buf);
            lv_obj_set_style_text_color(cap_label,
                lv_color_hex(isCommandable ? COL_SAFE : 0x8b949e), 0);
            lv_obj_set_width(cap_label, 148);
            lv_label_set_long_mode(cap_label, LV_LABEL_LONG_WRAP);
            lv_obj_set_pos(cap_label, 0, capY);
            lv_obj_update_layout(cap_label);
            capY += lv_obj_get_height(cap_label) + 4;
        }
    }

    // Divider
    int dividerY = capY + 6;
    lv_obj_t *divider = lv_obj_create(btn_panel);
    lv_obj_set_size(divider, 148, 1);
    lv_obj_set_pos(divider, 0, dividerY);
    lv_obj_set_style_bg_color(divider, lv_color_hex(0x30363d), 0);
    lv_obj_set_style_border_width(divider, 0, 0);
    lv_obj_set_style_radius(divider, 0, 0);

    // COMMANDS header
    lv_obj_t *cmd_title = lv_label_create(btn_panel);
    lv_label_set_text(cmd_title, "COMMANDS");
    lv_obj_set_style_text_color(cmd_title, lv_color_hex(COL_ACCENT), 0);
    lv_obj_set_pos(cmd_title, 0, dividerY + 8);

    int lastBtnY = dividerY + 28;  // tracked so error label can sit below buttons

    if (currentRole != ROLE_OPERATOR) {
    // Viewer: no buttons, just a notice
    lv_obj_t *view_only = lv_label_create(btn_panel);
    lv_label_set_text(view_only, "View only");
    lv_obj_set_style_text_color(view_only, lv_color_hex(0x8b949e), 0);
    lv_obj_set_style_text_align(view_only, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(view_only, 148);
    lv_label_set_long_mode(view_only, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(view_only, 0, dividerY + 28);
    } else if (cmdCount == 0) {
     lv_obj_t *no_cmd = lv_label_create(btn_panel);
     lv_label_set_text(no_cmd, "Autonomous mode only");
     lv_obj_set_style_text_color(no_cmd, lv_color_hex(0x8b949e), 0);
     lv_obj_set_style_text_align(no_cmd, LV_TEXT_ALIGN_CENTER, 0);
     lv_obj_set_width(no_cmd, 148);
     lv_label_set_long_mode(no_cmd, LV_LABEL_LONG_WRAP);
     lv_obj_set_pos(no_cmd, 0, dividerY + 28);
    } else {
     int startY = dividerY + 28;
     int btnH   = 34;
     int btnGap = 5;

    for (int i = 0; i < cmdCount; i++) {
        lv_obj_t *btn = lv_btn_create(btn_panel);
        lv_obj_set_size(btn, 144, btnH);
        lv_obj_set_pos(btn, 0, startY + i * (btnH + btnGap));
        lv_obj_set_style_bg_color(btn, lv_color_hex(getButtonColor(cmdButtons[i].command)), 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(getButtonColor(cmdButtons[i].command) + 0x222222), LV_STATE_PRESSED);
        lv_obj_set_style_radius(btn, 6, 0);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_set_style_shadow_width(btn, 0, 0);

        const char* cmdLiteral = NULL;
        if      (strcmp(cmdButtons[i].command, "forward") == 0) cmdLiteral = "forward";
        else if (strcmp(cmdButtons[i].command, "reverse") == 0) cmdLiteral = "reverse";
        else if (strcmp(cmdButtons[i].command, "left")    == 0) cmdLiteral = "left";
        else if (strcmp(cmdButtons[i].command, "right")   == 0) cmdLiteral = "right";
        else if (strcmp(cmdButtons[i].command, "stop")    == 0) cmdLiteral = "stop";
        else if (strcmp(cmdButtons[i].command, "pick_object") == 0) cmdLiteral = "pick_object";
        if (cmdLiteral) {
            lv_obj_add_event_cb(btn, onCommandPressed, LV_EVENT_CLICKED, (void*)cmdLiteral);
        }

        lv_obj_t *btn_label = lv_label_create(btn);
        lv_label_set_text(btn_label, cmdButtons[i].label);
        lv_obj_set_style_text_color(btn_label, lv_color_hex(0xffffff), 0);
        lv_obj_center(btn_label);

        lastBtnY = startY + i * (btnH + btnGap) + btnH;
    }

    if (cmdCount > 2) {
    // Many buttons — no space in right panel, put label right of Back button
    err_label = lv_label_create(info_panel);
    lv_obj_set_style_text_align(err_label, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_width(err_label, 154);
    lv_obj_set_pos(err_label, 108, 194);
    } else {
    // Few buttons — enough space below them in right panel
    err_label = lv_label_create(btn_panel);
    lv_obj_set_style_text_align(err_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(err_label, 148);
    lv_obj_set_pos(err_label, 0, lastBtnY + 6);
    }
    lv_label_set_text(err_label, "");
    lv_obj_set_style_text_color(err_label, lv_color_hex(COL_DANGER), 0);
    lv_obj_set_style_text_font(err_label, &lv_font_montserrat_14, 0);
    lv_label_set_long_mode(err_label, LV_LABEL_LONG_WRAP);
    lv_obj_add_flag(err_label, LV_OBJ_FLAG_HIDDEN);
}
}