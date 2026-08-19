#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_ota_ops.h"
#include "sdkconfig.h"
#include "settings.h"

static const char *TAG = "settings";
const char *uf2_nvs_partition = "nvs";
const char *uf2_nvs_namespace = "configuration";
static nvs_handle_t my_handle;
static sys_param_t g_sys_param = {0};

static char s_active_kb_url[URL_SIZE];
static char s_active_kb_label[RFID_LABEL_SIZE] = "Default";

void settings_init_active_kb(void)
{
    snprintf(s_active_kb_url, sizeof(s_active_kb_url), "%s", g_sys_param.kb_url);
    snprintf(s_active_kb_label, sizeof(s_active_kb_label), "Default");
    ESP_LOGI(TAG, "Default KB: %s", s_active_kb_url);
}

bool settings_build_api_base_url(char *out, size_t out_len)
{
    if (!out || out_len == 0 || g_sys_param.kb_url[0] == '\0') {
        return false;
    }

    const char *marker = "/api/v1/kb/";
    const char *pos = strstr(g_sys_param.kb_url, marker);
    if (!pos) {
        return false;
    }

    size_t prefix_len = (size_t)(pos - g_sys_param.kb_url);
    int written = snprintf(out, out_len, "%.*s", (int)prefix_len, g_sys_param.kb_url);
    return written > 0 && (size_t)written < out_len;
}

bool settings_build_kb_query_url(const char *kb_id, char *out, size_t out_len)
{
    char api_base[URL_SIZE] = {0};
    if (!kb_id || kb_id[0] == '\0' || !settings_build_api_base_url(api_base, sizeof(api_base))) {
        return false;
    }

    int written = snprintf(out, out_len, "%s/api/v1/kb/%s/query", api_base, kb_id);
    return written > 0 && (size_t)written < out_len;
}

esp_err_t settings_set_active_kb(const char *label, const char *kb_url)
{
    if (!kb_url || kb_url[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }

    snprintf(s_active_kb_url, sizeof(s_active_kb_url), "%s", kb_url);
    snprintf(s_active_kb_label, sizeof(s_active_kb_label), "%s", label ? label : "KB");
    ESP_LOGI(TAG, "Active KB switched to %s (%s)", s_active_kb_label, s_active_kb_url);
    return ESP_OK;
}

esp_err_t settings_reset_active_kb_to_default(void)
{
    snprintf(s_active_kb_url, sizeof(s_active_kb_url), "%s", g_sys_param.kb_url);
    snprintf(s_active_kb_label, sizeof(s_active_kb_label), "Default");
    ESP_LOGI(TAG, "Active KB reset to default (%s)", s_active_kb_url);
    return ESP_OK;
}

const char *settings_get_active_kb_url(void)
{
    if (s_active_kb_url[0] == '\0') {
        return g_sys_param.kb_url;
    }
    return s_active_kb_url;
}

const char *settings_get_active_kb_label(void)
{
    return s_active_kb_label;
}

esp_err_t settings_factory_reset(void)
{
    const esp_partition_t *update_partition = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
    ESP_LOGI(TAG, "Switch to partition UF2");
    esp_ota_set_boot_partition(update_partition);
    esp_restart();
    return ESP_OK;
}

esp_err_t settings_read_parameter_from_nvs(void)
{
    esp_err_t ret = nvs_open_from_partition(uf2_nvs_partition, uf2_nvs_namespace, NVS_READONLY, &my_handle);
    if (ESP_ERR_NVS_NOT_FOUND == ret) {
        ESP_LOGI(TAG, "Credentials not found");
        goto err;
    }

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "nvs open failed (0x%x)", ret);
        goto err;
    }

    size_t len = sizeof(g_sys_param.ssid);
    ret = nvs_get_str(my_handle, "ssid", g_sys_param.ssid, &len);
    if (ret != ESP_OK || len == 0) {
        goto err;
    }

    len = sizeof(g_sys_param.password);
    ret = nvs_get_str(my_handle, "password", g_sys_param.password, &len);
    if (ret != ESP_OK || len == 0) {
        goto err;
    }

    len = sizeof(g_sys_param.key);
    ret = nvs_get_str(my_handle, "ChatGPT_key", g_sys_param.key, &len);
    if (ret != ESP_OK || len == 0) {
        goto err;
    }

    len = sizeof(g_sys_param.url);
    ret = nvs_get_str(my_handle, "Base_url", g_sys_param.url, &len);
    if (ret != ESP_OK || len == 0) {
        goto err;
    }

    len = sizeof(g_sys_param.kb_url);
    ret = nvs_get_str(my_handle, "KB_url", g_sys_param.kb_url, &len);
    if (ret != ESP_OK || len == 0) {
        goto err;
    }

    len = sizeof(g_sys_param.tts_voice);
    ret = nvs_get_str(my_handle, "tts_voice", g_sys_param.tts_voice, &len);
    if (ret != ESP_OK || len == 0) {
        strcpy(g_sys_param.tts_voice, CONFIG_TTS_VOICE);
    }

    nvs_close(my_handle);
    settings_init_active_kb();

    ESP_LOGI(TAG, "ssid: %s", g_sys_param.ssid);
    ESP_LOGI(TAG, "OpenAI key: configured");
    ESP_LOGI(TAG, "Base URL: %s", g_sys_param.url);
    ESP_LOGI(TAG, "Default KB URL: %s", g_sys_param.kb_url);
    return ESP_OK;

err:
    if (my_handle) {
        nvs_close(my_handle);
    }
    settings_factory_reset();
    return ret;
}

sys_param_t *settings_get_parameter(void)
{
    return &g_sys_param;
}
