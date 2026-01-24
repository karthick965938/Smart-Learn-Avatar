/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

#define THEME_DARK  0
#define THEME_LIGHT 1

/**
 * @brief Apply theme colors to factory_nvs LVGL
 * 
 * @param theme_type 0 for Dark, 1 for Light
 */
void app_theme_apply(uint8_t theme_type);

#ifdef __cplusplus
}
#endif
