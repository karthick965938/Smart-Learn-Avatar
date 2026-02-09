/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#pragma once

#include "esp_err.h"
#include "OpenAI.h"

#define SSID_SIZE 32
#define PASSWORD_SIZE 64
#define KEY_SIZE 165
#define URL_SIZE 256
#define VOICE_SIZE 32
#define THEME_SIZE 16

// Theme types
typedef enum {
    THEME_DARK = 0,
    THEME_LIGHT = 1
} theme_type_t;

typedef struct {
    char ssid[SSID_SIZE];             /* SSID of target AP. */
    char password[PASSWORD_SIZE];     /* Password of target AP. */
    char key[KEY_SIZE];               /* OpenAI key. */
    char url[URL_SIZE];               /* OpenAI Base url. */
    char kb_url[URL_SIZE];            /* Knowledge Base url. */
    
    // Voice and UI settings
    char tts_voice[VOICE_SIZE];       /* TTS voice (alloy, echo, fable, onyx, nova, shimmer). */
    uint8_t theme_type;               /* Theme type: 0=Dark, 1=Light. */
} sys_param_t;

esp_err_t settings_factory_reset(void);
esp_err_t settings_read_parameter_from_nvs(void);
sys_param_t *settings_get_parameter(void);
