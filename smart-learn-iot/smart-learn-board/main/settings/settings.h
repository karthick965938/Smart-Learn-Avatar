#pragma once

#include "esp_err.h"
#include "OpenAI.h"

#define SSID_SIZE 32
#define PASSWORD_SIZE 64
#define KEY_SIZE 165
#define URL_SIZE 256
#define VOICE_SIZE 32
#define RFID_UID_SIZE 16
#define RFID_LABEL_SIZE 24

typedef struct {
    char ssid[SSID_SIZE];
    char password[PASSWORD_SIZE];
    char key[KEY_SIZE];
    char url[URL_SIZE];
    char kb_url[URL_SIZE];
    char tts_voice[VOICE_SIZE];
    uint8_t theme_type;
} sys_param_t;

esp_err_t settings_factory_reset(void);
esp_err_t settings_read_parameter_from_nvs(void);
sys_param_t *settings_get_parameter(void);

void settings_init_active_kb(void);
bool settings_build_api_base_url(char *out, size_t out_len);
bool settings_build_kb_query_url(const char *kb_id, char *out, size_t out_len);
esp_err_t settings_set_active_kb(const char *label, const char *kb_url);
esp_err_t settings_reset_active_kb_to_default(void);
const char *settings_get_active_kb_url(void);
const char *settings_get_active_kb_label(void);
