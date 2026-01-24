/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

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
            // If it's a semi-transparent overlay (like ImageSetupTextBg), make it very faint
            if (lv_obj_get_style_bg_opa(obj, 0) < 255) {
                lv_obj_set_style_bg_opa(obj, 10, 0); 
            }
        } else {
            // Restore dark gradient for screens
            if (lv_obj_get_parent(obj) == NULL) {
                lv_obj_set_style_bg_grad_color(obj, lv_color_hex(0x1a1a2e), 0);
                lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_VER, 0);
            } else if (lv_obj_get_style_bg_opa(obj, 0) < 255) {
                lv_obj_set_style_bg_opa(obj, 100, 0); // Restore dark overlay
                lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), 0);
            }
        }
    }

    // 3. Handle Text Colors (including labels inside buttons)
    if (lv_obj_has_class(obj, &lv_label_class)) {
        lv_obj_set_style_text_color(obj, text_color, 0);
    }

    // 4. Recurse to children
    uint32_t i;
    for (i = 0; i < lv_obj_get_child_cnt(obj); i++) {
        apply_theme_to_obj_recursive(lv_obj_get_child(obj, i), bg_color, text_color, is_light);
    }
}

void app_theme_apply(uint8_t theme_type)
{
    bool is_light = (theme_type == THEME_LIGHT);
    ESP_LOGI(TAG, "Applying theme - Type:%s", is_light ? "LIGHT" : "DARK");
    
    lv_color_t bg_color = is_light ? lv_color_hex(0xFFFFFF) : lv_color_hex(0x000000);
    lv_color_t text_color = is_light ? lv_color_hex(0x000000) : lv_color_hex(0xFFFFFF);
    lv_color_t button_color = lv_color_hex(0x04B900); // Default green
    
    lv_disp_t *disp = lv_disp_get_default();
    if (disp == NULL) return;
    
    lv_theme_t *theme = lv_theme_default_init(
        disp,
        button_color,
        lv_color_hex(0xD24B09),
        is_light,
        LV_FONT_DEFAULT
    );
    
    if (theme != NULL) {
        lv_disp_set_theme(disp, theme);
        
        // Wait for LVGL to finish any pending operations
        lv_timer_handler();

        // 1. Apply to known global screens
        if (ui_ScreenSetup != NULL) {
            ESP_LOGI(TAG, "Applying theme to ui_ScreenSetup");
            apply_theme_to_obj_recursive(ui_ScreenSetup, bg_color, text_color, is_light);
        } else {
            ESP_LOGW(TAG, "ui_ScreenSetup is NULL!");
        }
        
        // 2. Apply to current active screen as well
        lv_obj_t *scr = lv_scr_act();
        if (scr) {
            if (scr != ui_ScreenSetup) {
                ESP_LOGI(TAG, "Applying theme to active screen (different from ui_ScreenSetup)");
                apply_theme_to_obj_recursive(scr, bg_color, text_color, is_light);
            }
            // Force refresh of the active screen
            lv_obj_invalidate(scr);
            lv_refr_now(NULL);
        }
        
        ESP_LOGI(TAG, "Theme applied successfully");
    }
}
