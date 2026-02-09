/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <inttypes.h>
#include "app_theme.h"
#include "esp_log.h"
#include "ui.h"

static const char *TAG = "app_theme";

static void apply_theme_to_obj_recursive(lv_obj_t *obj, lv_color_t bg_color, lv_color_t text_color, bool is_light)
{
    if (obj == NULL) return;

    extern const lv_obj_class_t lv_btn_class;
    extern const lv_obj_class_t lv_label_class;

    // 1. Handle Buttons
    if (lv_obj_has_class(obj, &lv_btn_class)) {
        lv_obj_set_style_bg_color(obj, lv_color_hex(0x04B900), 0);
        lv_obj_set_style_bg_opa(obj, 255, 0);
        lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_NONE, 0); // Remove any gradients on buttons
    } 
    // 2. Handle background colors and gradients for non-button objects
    else if (lv_obj_get_parent(obj) == NULL || lv_obj_get_style_bg_opa(obj, 0) > 10) {
        lv_obj_set_style_bg_color(obj, bg_color, 0);
        
        // Remove or update gradients for light mode to stay clean
        if (is_light) {
            lv_obj_set_style_bg_grad_color(obj, bg_color, 0);
            lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_NONE, 0);
        } else {
            // Restore dark gradient for dark mode if it's a screen
            if (lv_obj_get_parent(obj) == NULL) {
                lv_obj_set_style_bg_grad_color(obj, lv_color_hex(0x1a1a2e), 0);
                lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_VER, 0);
            }
        }
    }

    // 3. Handle Text Colors (including labels inside buttons)
    if (lv_obj_has_class(obj, &lv_label_class)) {
        // Special case: "Thinking ...", "Listening ...", and STT content labels 
        // should stay #D24B09 as requested.
        if (obj == ui_LabelListenSpeak || obj == ui_LabelReplyQuestion) {
            lv_obj_set_style_text_color(obj, lv_color_hex(0xD24B09), 0);
        } else {
            lv_obj_set_style_text_color(obj, text_color, 0);
        }
    }

    // 4. Recurse to children
    uint32_t i;
    for (i = 0; i < lv_obj_get_child_cnt(obj); i++) {
        apply_theme_to_obj_recursive(lv_obj_get_child(obj, i), bg_color, text_color, is_light);
    }
}

lv_color_t app_theme_get_bg_color(sys_param_t *param)
{
    if (param == NULL || param->theme_type == THEME_DARK) {
        return lv_color_hex(0x000000); // Dark: Black background
    } else {
        return lv_color_hex(0xFFFFFF); // Light: White background
    }
}

lv_color_t app_theme_get_text_color(sys_param_t *param)
{
    if (param == NULL || param->theme_type == THEME_DARK) {
        return lv_color_hex(0xFFFFFF); // Dark: White text
    } else {
        return lv_color_hex(0x000000); // Light: Black text
    }
}

lv_color_t app_theme_get_button_color(sys_param_t *param)
{
    return lv_color_hex(0x04B900); // Default green
}

void app_theme_apply(sys_param_t *param)
{
    if (param == NULL) {
        ESP_LOGE(TAG, "Cannot apply theme: NULL parameter");
        return;
    }
    
    bool is_light = (param->theme_type == THEME_LIGHT);
    ESP_LOGI(TAG, "Applying theme - Type:%s", is_light ? "LIGHT" : "DARK");
    
    // Get colors
    lv_color_t bg_color = app_theme_get_bg_color(param);
    lv_color_t text_color = app_theme_get_text_color(param);
    lv_color_t button_color = app_theme_get_button_color(param);
    
    // Apply theme to default display
    lv_disp_t *disp = lv_disp_get_default();
    if (disp == NULL) {
        ESP_LOGE(TAG, "No default display found");
        return;
    }
    
    // Update default theme
    lv_theme_t *theme = lv_theme_default_init(
        disp,
        button_color,  // Primary color
        lv_color_hex(0xD24B09),  // Secondary color
        is_light,  // Light/dark mode
        LV_FONT_DEFAULT
    );
    
    if (theme != NULL) {
        lv_disp_set_theme(disp, theme);
        
        // Traverse all known screens and apply colors recursively
        // This is necessary because SquareLine Studio generates hardcoded styles
        lv_obj_t *screens[] = {ui_ScreenSetup, ui_ScreenWifiReset, ui_ScreenListen, ui_ScreenReset};
        for (int i = 0; i < sizeof(screens) / sizeof(lv_obj_t *); i++) {
            if (screens[i] != NULL) {
                apply_theme_to_obj_recursive(screens[i], bg_color, text_color, is_light);
            }
        }
        
        // Force refresh of the active screen
        lv_obj_t *scr = lv_scr_act();
        if (scr) {
            lv_obj_invalidate(scr);
        }
        
        ESP_LOGI(TAG, "Theme applied successfully to all screens");
    } else {
        ESP_LOGE(TAG, "Failed to create theme");
    }
}
