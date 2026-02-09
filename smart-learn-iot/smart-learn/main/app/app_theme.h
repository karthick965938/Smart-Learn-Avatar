/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#pragma once

#include "lvgl.h"
#include "settings.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Apply theme colors from settings to LVGL
 * 
 * @param param System parameters containing theme settings
 */
void app_theme_apply(sys_param_t *param);

/**
 * @brief Get background color from settings
 * 
 * @param param System parameters
 * @return lv_color_t Background color
 */
lv_color_t app_theme_get_bg_color(sys_param_t *param);

/**
 * @brief Get text color from settings
 * 
 * @param param System parameters
 * @return lv_color_t Text color
 */
lv_color_t app_theme_get_text_color(sys_param_t *param);

/**
 * @brief Get button/primary color from settings
 * 
 * @param param System parameters
 * @return lv_color_t Button color
 */
lv_color_t app_theme_get_button_color(sys_param_t *param);

#ifdef __cplusplus
}
#endif
