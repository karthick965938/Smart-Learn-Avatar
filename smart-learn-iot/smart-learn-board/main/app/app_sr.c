#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "app_sr.h"
#include "esp_process_sdkconfig.h"
#include "esp_afe_sr_models.h"
#include "esp_mn_models.h"
#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "esp_afe_sr_iface.h"
#include "esp_mn_iface.h"
#include "model_path.h"
#include "bsp_board.h"
#include "app_audio.h"
#include "app_wifi.h"

static const char *TAG = "app_sr";

static esp_afe_sr_iface_t *afe_handle = NULL;
static srmodel_list_t *models = NULL;
static bool manul_detect_flag = false;

sr_data_t *g_sr_data = NULL;

#define I2S_CHANNEL_NUM 2

extern bool record_flag;
extern uint32_t record_total_len;

static void audio_feed_task(void *arg)
{
    size_t bytes_read = 0;
    esp_afe_sr_data_t *afe_data = (esp_afe_sr_data_t *)arg;
    int audio_chunksize = afe_handle->get_feed_chunksize(afe_data);
    int feed_channel = 3;

    int16_t *audio_buffer = heap_caps_malloc(audio_chunksize * sizeof(int16_t) * feed_channel,
                                             MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    assert(audio_buffer);
    g_sr_data->afe_in_buffer = audio_buffer;

    while (true) {
        if (g_sr_data->event_group && xEventGroupGetBits(g_sr_data->event_group)) {
            xEventGroupSetBits(g_sr_data->event_group, FEED_DELETED);
            vTaskDelete(NULL);
        }

        bsp_i2s_read((char *)audio_buffer, audio_chunksize * I2S_CHANNEL_NUM * sizeof(int16_t),
                     &bytes_read, portMAX_DELAY);

        for (int i = audio_chunksize - 1; i >= 0; i--) {
            audio_buffer[i * 3 + 2] = 0;
            audio_buffer[i * 3 + 1] = audio_buffer[i * 2];
            audio_buffer[i * 3 + 0] = audio_buffer[i * 2];
        }

        if (WIFI_STATUS_CONNECTED_OK == wifi_connected_already()) {
            afe_handle->feed(afe_data, audio_buffer);
        }
        audio_record_save(audio_buffer, audio_chunksize);
    }
}

static void audio_detect_task(void *arg)
{
    static afe_vad_state_t local_state;
    static uint8_t frame_keep = 0;
    bool detect_flag = false;
    esp_afe_sr_data_t *afe_data = arg;

    while (true) {
        if (NEED_DELETE && xEventGroupGetBits(g_sr_data->event_group)) {
            xEventGroupSetBits(g_sr_data->event_group, DETECT_DELETED);
            vTaskDelete(g_sr_data->handle_task);
            vTaskDelete(NULL);
        }

        afe_fetch_result_t *res = afe_handle->fetch(afe_data);
        if (!res || res->ret_value == ESP_FAIL) {
            continue;
        }

        if (res->wakeup_state == WAKENET_DETECTED) {
            sr_result_t result = {
                .wakenet_mode = WAKENET_DETECTED,
                .state = ESP_MN_STATE_DETECTING,
                .command_id = 0,
            };
            xQueueSend(g_sr_data->result_que, &result, 0);
        } else if (res->wakeup_state == WAKENET_CHANNEL_VERIFIED || manul_detect_flag) {
            detect_flag = true;
            if (manul_detect_flag) {
                manul_detect_flag = false;
                sr_result_t result = {
                    .wakenet_mode = WAKENET_DETECTED,
                    .state = ESP_MN_STATE_DETECTING,
                    .command_id = 0,
                };
                xQueueSend(g_sr_data->result_que, &result, 0);
            }
            frame_keep = 0;
            g_sr_data->afe_handle->disable_wakenet(afe_data);
        }

        if (detect_flag) {
            if (local_state != res->vad_state) {
                local_state = res->vad_state;
                frame_keep = 0;
            } else {
                frame_keep++;
            }

            if ((100 == frame_keep) && (AFE_VAD_SILENCE == res->vad_state)) {
                sr_result_t result = {
                    .wakenet_mode = WAKENET_NO_DETECT,
                    .state = ESP_MN_STATE_TIMEOUT,
                    .command_id = 0,
                };
                xQueueSend(g_sr_data->result_que, &result, 0);
                g_sr_data->afe_handle->enable_wakenet(afe_data);
                detect_flag = false;
            }
        }
    }
}

esp_err_t app_sr_set_language(sr_language_t new_lang)
{
    ESP_RETURN_ON_FALSE(NULL != g_sr_data, ESP_ERR_INVALID_STATE, TAG, "SR is not running");

    if (new_lang == g_sr_data->lang) {
        return ESP_OK;
    }
    g_sr_data->lang = new_lang;

    if (g_sr_data->model_data) {
        g_sr_data->multinet->destroy(g_sr_data->model_data);
    }
    char *wn_name = esp_srmodel_filter(models, ESP_WN_PREFIX, "");
    g_sr_data->afe_handle->set_wakenet(g_sr_data->afe_data, wn_name);
    return ESP_OK;
}

esp_err_t app_sr_start(bool record_en)
{
    (void)record_en;
    esp_err_t ret = ESP_OK;
    ESP_RETURN_ON_FALSE(NULL == g_sr_data, ESP_ERR_INVALID_STATE, TAG, "SR already running");

    g_sr_data = heap_caps_calloc(1, sizeof(sr_data_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    ESP_RETURN_ON_FALSE(NULL != g_sr_data, ESP_ERR_NO_MEM, TAG, "Failed create sr data");

    g_sr_data->result_que = xQueueCreate(3, sizeof(sr_result_t));
    ESP_GOTO_ON_FALSE(NULL != g_sr_data->result_que, ESP_ERR_NO_MEM, err, TAG, "queue");

    g_sr_data->event_group = xEventGroupCreate();
    ESP_GOTO_ON_FALSE(NULL != g_sr_data->event_group, ESP_ERR_NO_MEM, err, TAG, "event group");

    models = esp_srmodel_init("model");
    afe_handle = (esp_afe_sr_iface_t *)&ESP_AFE_SR_HANDLE;
    afe_config_t afe_config = AFE_CONFIG_DEFAULT();
    afe_config.wakenet_model_name = esp_srmodel_filter(models, ESP_WN_PREFIX, NULL);
    afe_config.aec_init = false;

    esp_afe_sr_data_t *afe_data = afe_handle->create_from_config(&afe_config);
    g_sr_data->afe_handle = afe_handle;
    g_sr_data->afe_data = afe_data;
    g_sr_data->lang = SR_LANG_MAX;

    ret = app_sr_set_language(SR_LANG_EN);
    ESP_GOTO_ON_FALSE(ESP_OK == ret, ESP_FAIL, err, TAG, "language");

    BaseType_t ret_val = xTaskCreatePinnedToCore(audio_feed_task, "Feed Task", 8 * 1024, afe_data, 5,
                                                 &g_sr_data->feed_task, 0);
    ESP_GOTO_ON_FALSE(pdPASS == ret_val, ESP_FAIL, err, TAG, "feed task");

    ret_val = xTaskCreatePinnedToCore(audio_detect_task, "Detect Task", 10 * 1024, afe_data, 5,
                                      &g_sr_data->detect_task, 1);
    ESP_GOTO_ON_FALSE(pdPASS == ret_val, ESP_FAIL, err, TAG, "detect task");

    ret_val = xTaskCreatePinnedToCore(sr_handler_task, "SR Handler Task", 8 * 1024, NULL, 5,
                                      &g_sr_data->handle_task, 0);
    ESP_GOTO_ON_FALSE(pdPASS == ret_val, ESP_FAIL, err, TAG, "handler task");

    audio_record_init();
    return ESP_OK;

err:
    app_sr_stop();
    return ret;
}

esp_err_t app_sr_stop(void)
{
    ESP_RETURN_ON_FALSE(NULL != g_sr_data, ESP_ERR_INVALID_STATE, TAG, "SR is not running");
    xEventGroupSetBits(g_sr_data->event_group, NEED_DELETE);
    xEventGroupWaitBits(g_sr_data->event_group, NEED_DELETE | FEED_DELETED | DETECT_DELETED | HANDLE_DELETED,
                        1, 1, portMAX_DELAY);

    if (g_sr_data->result_que) {
        vQueueDelete(g_sr_data->result_que);
    }
    if (g_sr_data->event_group) {
        vEventGroupDelete(g_sr_data->event_group);
    }
    if (g_sr_data->model_data) {
        g_sr_data->multinet->destroy(g_sr_data->model_data);
    }
    if (g_sr_data->afe_data) {
        g_sr_data->afe_handle->destroy(g_sr_data->afe_data);
    }
    if (g_sr_data->afe_in_buffer) {
        heap_caps_free(g_sr_data->afe_in_buffer);
    }
    heap_caps_free(g_sr_data);
    g_sr_data = NULL;
    return ESP_OK;
}

esp_err_t app_sr_get_result(sr_result_t *result, TickType_t xTicksToWait)
{
    ESP_RETURN_ON_FALSE(NULL != g_sr_data, ESP_ERR_INVALID_STATE, TAG, "SR is not running");
    xQueueReceive(g_sr_data->result_que, result, xTicksToWait);
    return ESP_OK;
}

esp_err_t app_sr_start_once(void)
{
    ESP_RETURN_ON_FALSE(NULL != g_sr_data, ESP_ERR_INVALID_STATE, TAG, "SR is not running");
    manul_detect_flag = true;
    return ESP_OK;
}
