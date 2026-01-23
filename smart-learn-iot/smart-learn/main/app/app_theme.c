/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <inttypes.h>
#include "app_theme.h"
#include "esp_log.h"

static const char *TAG = "app_theme";

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
    
    ESP_LOGI(TAG, "Applying theme - Type:%d", param->theme_type);
    
    // Get colors
    lv_color_t button_color = app_theme_get_button_color(param);
    
    // Apply theme to default display
    lv_disp_t *disp = lv_disp_get_default();
    if (disp == NULL) {
        ESP_LOGE(TAG, "No default display found");
        return;
    }
    
    // Get or create theme
    lv_theme_t *theme = lv_theme_default_init(
        disp,
        button_color,  // Primary color
        lv_color_hex(0xD24B09),  // Secondary color
        param->theme_type == THEME_LIGHT ? true : false,  // Light/dark mode
        LV_FONT_DEFAULT
    );
    
    if (theme != NULL) {
        lv_disp_set_theme(disp, theme);
        
        // Explicitly set the screen background color to ensure it matches precisely
        lv_obj_t *scr = lv_scr_act();
        if (scr) {
            lv_obj_set_style_bg_color(scr, app_theme_get_bg_color(param), 0);
        }
        
        ESP_LOGI(TAG, "Theme applied successfully");
    } else {
        ESP_LOGE(TAG, "Failed to create theme");
    }
}
