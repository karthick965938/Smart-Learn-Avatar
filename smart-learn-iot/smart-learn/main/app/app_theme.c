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
    if (param == NULL) {
        return lv_color_hex(0x000000); // Default black
    }
    
    // Apply theme-specific defaults if custom colors not set
    switch (param->theme_type) {
        case THEME_LIGHT:
            return param->bg_color != 0 ? lv_color_hex(param->bg_color) : lv_color_hex(0xFFFFFF);
        case THEME_CUSTOM:
            return lv_color_hex(param->bg_color != 0 ? param->bg_color : 0x1a1a2e);
        case THEME_DARK:
        default:
            return lv_color_hex(param->bg_color != 0 ? param->bg_color : 0x000000);
    }
}

lv_color_t app_theme_get_text_color(sys_param_t *param)
{
    if (param == NULL) {
        return lv_color_hex(0xFFFFFF); // Default white
    }
    
    // Apply theme-specific defaults if custom colors not set
    switch (param->theme_type) {
        case THEME_LIGHT:
            return param->text_color != 0 ? lv_color_hex(param->text_color) : lv_color_hex(0x000000);
        case THEME_DARK:
        case THEME_CUSTOM:
        default:
            return param->text_color != 0 ? lv_color_hex(param->text_color) : lv_color_hex(0xFFFFFF);
    }
}

lv_color_t app_theme_get_button_color(sys_param_t *param)
{
    if (param == NULL) {
        return lv_color_hex(0x04B900); // Default green
    }
    
    return lv_color_hex(param->button_color != 0 ? param->button_color : 0x04B900);
}

void app_theme_apply(sys_param_t *param)
{
    if (param == NULL) {
        ESP_LOGE(TAG, "Cannot apply theme: NULL parameter");
        return;
    }
    
    ESP_LOGI(TAG, "Applying theme - Type:%d, BG:0x%06" PRIX32 ", Text:0x%06" PRIX32 ", Button:0x%06" PRIX32,
             param->theme_type, param->bg_color, param->text_color, param->button_color);
    
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
        lv_color_hex(0xD24B09),  // Secondary color (keep orange)
        param->theme_type == THEME_LIGHT ? true : false,  // Light/dark mode
        LV_FONT_DEFAULT
    );
    
    if (theme != NULL) {
        lv_disp_set_theme(disp, theme);
        ESP_LOGI(TAG, "Theme applied successfully");
    } else {
        ESP_LOGE(TAG, "Failed to create theme");
    }
}
