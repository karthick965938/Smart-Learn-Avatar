#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "cJSON.h"
#include "app_rfid.h"
#include "app_ui_ctrl.h"
#include "bsp_rfid.h"
#include "settings.h"

static const char *TAG = "app_rfid";

typedef struct {
    char uid[RFID_UID_SIZE];
} rfid_scan_task_args_t;

static esp_err_t http_collect_response(esp_http_client_event_t *evt)
{
    if (evt->event_id == HTTP_EVENT_ON_DATA) {
        if (!esp_http_client_is_chunked_response(evt->client)) {
            char **output_buffer = (char **)evt->user_data;
            if (*output_buffer == NULL) {
                *output_buffer = (char *)malloc(evt->data_len + 1);
                if (*output_buffer) {
                    memcpy(*output_buffer, evt->data, evt->data_len);
                    (*output_buffer)[evt->data_len] = 0;
                }
            } else {
                int current_len = strlen(*output_buffer);
                *output_buffer = (char *)realloc(*output_buffer, current_len + evt->data_len + 1);
                if (*output_buffer) {
                    memcpy(*output_buffer + current_len, evt->data, evt->data_len);
                    (*output_buffer)[current_len + evt->data_len] = 0;
                }
            }
        }
    }
    return ESP_OK;
}

static bool build_rfid_scan_url(char *out, size_t out_len)
{
    char api_base[URL_SIZE] = {0};
    if (!settings_build_api_base_url(api_base, sizeof(api_base))) {
        return false;
    }

    int written = snprintf(out, out_len, "%s/api/v1/iot/rfid/scan", api_base);
    return written > 0 && (size_t)written < out_len;
}

static void apply_scan_response(const char *uid, const char *response_body)
{
    if (!response_body || response_body[0] == '\0') {
        ESP_LOGW(TAG, "Empty RFID scan response for uid=%s", uid);
        return;
    }

    cJSON *json = cJSON_Parse(response_body);
    if (!json) {
        ESP_LOGW(TAG, "Failed to parse RFID scan JSON for uid=%s", uid);
        return;
    }

    cJSON *assigned_item = cJSON_GetObjectItemCaseSensitive(json, "assigned");
    cJSON *kb_name_item = cJSON_GetObjectItemCaseSensitive(json, "kb_name");
    cJSON *kb_url_item = cJSON_GetObjectItemCaseSensitive(json, "kb_url");
    cJSON *kb_id_item = cJSON_GetObjectItemCaseSensitive(json, "kb_id");

    bool assigned = cJSON_IsTrue(assigned_item);
    const char *kb_name = cJSON_IsString(kb_name_item) ? kb_name_item->valuestring : NULL;
    const char *kb_url = cJSON_IsString(kb_url_item) ? kb_url_item->valuestring : NULL;
    const char *kb_id = cJSON_IsString(kb_id_item) ? kb_id_item->valuestring : NULL;

    char built_url[URL_SIZE] = {0};
    if (!kb_url && kb_id) {
        settings_build_kb_query_url(kb_id, built_url, sizeof(built_url));
        kb_url = built_url[0] ? built_url : NULL;
    }

    if (assigned && kb_url) {
        settings_set_active_kb(kb_name ? kb_name : "KB", kb_url);
        ui_ctrl_set_kb_label(settings_get_active_kb_label());
        ESP_LOGI(TAG, "RFID %s -> KB %s", uid, kb_url);
    } else {
        settings_reset_active_kb_to_default();
        ui_ctrl_set_kb_label("Unassigned");
        ESP_LOGW(TAG, "RFID %s not assigned; using default KB", uid);
    }

    cJSON_Delete(json);
}

static void rfid_scan_task(void *arg)
{
    rfid_scan_task_args_t *args = (rfid_scan_task_args_t *)arg;
    char scan_url[URL_SIZE] = {0};
    char post_data[64];
    char *response_buffer = NULL;

    if (!build_rfid_scan_url(scan_url, sizeof(scan_url))) {
        ESP_LOGW(TAG, "Could not derive RFID scan URL from default KB_url");
        free(args);
        vTaskDelete(NULL);
        return;
    }

    snprintf(post_data, sizeof(post_data), "{\"uid\":\"%s\"}", args->uid);
    ESP_LOGI(TAG, "RFID scan API: %s uid=%s", scan_url, args->uid);

    esp_http_client_config_t config = {
        .url = scan_url,
        .event_handler = http_collect_response,
        .user_data = &response_buffer,
        .buffer_size = 2048,
        .timeout_ms = 8000,
        .disable_auto_redirect = true,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "RFID scan API HTTP %d", status);
        if (status == 200 && response_buffer) {
            apply_scan_response(args->uid, response_buffer);
        }
    } else {
        ESP_LOGW(TAG, "RFID scan API failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    free(response_buffer);
    free(args);
    vTaskDelete(NULL);
}

static void handle_rfid_card(const char *uid_hex)
{
    rfid_scan_task_args_t *args = calloc(1, sizeof(rfid_scan_task_args_t));
    if (!args) {
        return;
    }
    snprintf(args->uid, sizeof(args->uid), "%s", uid_hex);
    xTaskCreate(rfid_scan_task, "rfid_scan", 6144, args, 2, NULL);
}

static void on_rfid_card(const char *uid_hex, void *user_data)
{
    (void)user_data;
    handle_rfid_card(uid_hex);
}

esp_err_t app_rfid_start(void)
{
    ESP_LOGI(TAG, "RFID scan handler started (API-driven KB switching)");
    return bsp_rfid_start_task(on_rfid_card, NULL);
}
