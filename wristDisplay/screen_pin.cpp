#include "screen_pin.h"
#include "screen_manager.h"
#include "config.h"
#include <string.h>

// --- Colours ---
#define COL_BG          0x0d1117
#define COL_PANEL       0x161b22
#define COL_ACCENT      0x00d4ff
#define COL_TEXT        0xe6edf3
#define COL_BTN         0x21262d
#define COL_BTN_PRESS   0x30363d
#define COL_ERROR       0xff3333
#define COL_SUCCESS     0x00ff88

// --- State ---
static char enteredPin[5];   // 4 digits + null terminator
static int  pinLength = 0;
static lv_obj_t *dot_labels[4];  // the **** display
static lv_obj_t *status_label;   // "Enter PIN" / "Wrong PIN"

// --- Forward declarations ---
static void onKeyPressed(lv_event_t *e);
static void onDeletePressed(lv_event_t *e);
static void updateDots();
static void checkPin();

// --- Update the dot display ---
static void updateDots() {
    for (int i = 0; i < 4; i++) {
        if (i < pinLength) {
            lv_label_set_text(dot_labels[i], LV_SYMBOL_BULLET);
            lv_obj_set_style_text_color(dot_labels[i], lv_color_hex(COL_ACCENT), 0);
        } else {
            lv_label_set_text(dot_labels[i], "—");
            lv_obj_set_style_text_color(dot_labels[i], lv_color_hex(0x30363d), 0);
        }
    }
}

// --- Check entered PIN ---
static void checkPin() {
    enteredPin[pinLength] = '\0';
    if (strcmp(enteredPin, WORKER_PIN) == 0) {
        lv_label_set_text(status_label, "Access granted");
        lv_obj_set_style_text_color(status_label, lv_color_hex(COL_SUCCESS), 0);
        // Small delay so user sees the success state, then switch
        lv_timer_t *timer = lv_timer_create([](lv_timer_t *t) {
            switchTo(SCREEN_ROBOT_LIST);
            lv_timer_del(t);
        }, 600, NULL);
    } else {
        lv_label_set_text(status_label, "Wrong PIN — try again");
        lv_obj_set_style_text_color(status_label, lv_color_hex(COL_ERROR), 0);
        // Reset after short delay
        lv_timer_t *timer = lv_timer_create([](lv_timer_t *t) {
            pinLength = 0;
            memset(enteredPin, 0, sizeof(enteredPin));
            updateDots();
            lv_label_set_text(status_label, "Enter PIN");
            lv_obj_set_style_text_color(status_label, lv_color_hex(COL_TEXT), 0);
            lv_timer_del(t);
        }, 1000, NULL);
    }
}

// --- Key button press handler ---
static void onKeyPressed(lv_event_t *e) {
    if (pinLength >= 4) return;
    const char *digit = (const char *)lv_event_get_user_data(e);
    enteredPin[pinLength] = digit[0];
    pinLength++;
    updateDots();
    if (pinLength == 4) checkPin();
}

// --- Delete button press handler ---
static void onDeletePressed(lv_event_t *e) {
    if (pinLength > 0) {
        pinLength--;
        enteredPin[pinLength] = '\0';
        updateDots();
        // Reset status text if it was showing an error
        lv_label_set_text(status_label, "Enter PIN");
        lv_obj_set_style_text_color(status_label, lv_color_hex(COL_TEXT), 0);
    }
}

void createPinScreen() {
    // Reset state
    pinLength = 0;
    memset(enteredPin, 0, sizeof(enteredPin));

    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(COL_BG), 0);

    // --- Header ---
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
    lv_label_set_text(sub, "Worker Authentication");
    lv_obj_set_style_text_color(sub, lv_color_hex(0x8b949e), 0);
    lv_obj_align(sub, LV_ALIGN_RIGHT_MID, -12, 0);

    // Accent line
    lv_obj_t *line = lv_obj_create(scr);
    lv_obj_set_size(line, SCREEN_WIDTH, 2);
    lv_obj_set_pos(line, 0, 44);
    lv_obj_set_style_bg_color(line, lv_color_hex(COL_ACCENT), 0);
    lv_obj_set_style_border_width(line, 0, 0);
    lv_obj_set_style_radius(line, 0, 0);

    // --- Status label ---
    status_label = lv_label_create(scr);
    lv_label_set_text(status_label, "Enter PIN");
    lv_obj_set_style_text_color(status_label, lv_color_hex(COL_TEXT), 0);
    lv_obj_align(status_label, LV_ALIGN_TOP_MID, 0, 58);

    // --- PIN dot display (4 slots) ---
    int dotSpacing = 48;
    int dotStartX  = (SCREEN_WIDTH / 2) - (dotSpacing * 3 / 2) - 12;

    for (int i = 0; i < 4; i++) {
        dot_labels[i] = lv_label_create(scr);
        lv_label_set_text(dot_labels[i], "—");
        lv_obj_set_style_text_color(dot_labels[i], lv_color_hex(0x30363d), 0);
        lv_obj_set_style_text_font(dot_labels[i], &lv_font_montserrat_14, 0);
        lv_obj_set_pos(dot_labels[i], dotStartX + i * dotSpacing, 82);
    }

    // --- Keypad ---
    // Layout: 3 columns x 4 rows  [1][2][3] / [4][5][6] / [7][8][9] / [DEL][0][OK(auto)]
    const char* keys[] = {"1","2","3","4","5","6","7","8","9","DEL","0"};
    int btnW    = 90;
    int btnH    = 44;
    int padX    = 6;
    int padY    = 6;
    int gridW   = 3 * btnW + 2 * padX;
    int startX  = (SCREEN_WIDTH - gridW) / 2;
    int startY  = 130;

    for (int i = 0; i < 11; i++) {
        int col = i % 3;
        int row = i / 3;

        lv_obj_t *btn = lv_btn_create(scr);
        lv_obj_set_size(btn, btnW, btnH);
        lv_obj_set_pos(btn, startX + col * (btnW + padX), startY + row * (btnH + padY));
        lv_obj_set_style_radius(btn, 8, 0);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_set_style_shadow_width(btn, 0, 0);

        bool isDel = (strcmp(keys[i], "DEL") == 0);

        lv_obj_set_style_bg_color(btn, lv_color_hex(isDel ? 0x3d1c1c : COL_BTN), 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(isDel ? 0x5a2a2a : COL_BTN_PRESS), LV_STATE_PRESSED);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, keys[i]);
        lv_obj_set_style_text_color(lbl, lv_color_hex(isDel ? COL_ERROR : COL_TEXT), 0);
        lv_obj_center(lbl);

        if (isDel) {
            lv_obj_add_event_cb(btn, onDeletePressed, LV_EVENT_CLICKED, NULL);
        } else {
            lv_obj_add_event_cb(btn, onKeyPressed, LV_EVENT_CLICKED, (void*)keys[i]);
        }
    }
}
