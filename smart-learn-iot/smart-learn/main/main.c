/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_check.h"
#include "nvs_flash.h"
#include "app_ui_ctrl.h"
#include "OpenAI.h"
#include "audio_player.h"
#include "app_sr.h"
#include "bsp/esp-bsp.h"
#include "bsp_board.h"
#include "app_audio.h"
#include "app_wifi.h"
#include "app_theme.h"
#include "settings.h"
#include "esp_http_client.h"
#include "cJSON.h"
#include "esp_crt_bundle.h"

#define SCROLL_START_DELAY_S            (1.5)
#define LISTEN_SPEAK_PANEL_DELAY_MS     2000
#define SERVER_ERROR                    "server_error"
#define INVALID_REQUEST_ERROR           "invalid_request_error"
#define SORRY_CANNOT_UNDERSTAND         "Sorry, I can't understand."
#define API_KEY_NOT_VALID               "API Key is not valid"

static char *TAG = "app_main";
static sys_param_t *sys_param = NULL;

esp_err_t _http_event_handle(esp_http_client_event_t *evt) {
    if (evt->event_id == HTTP_EVENT_ON_DATA) {
        if (!esp_http_client_is_chunked_response(evt->client)) {
            // Append data to buffer if not chunked, or handle chunked...
            // For simplicity, we assume the buffer in user_data is large enough or dynamic.
            // Here we just accept small responses fitting into one buffer for this demo
            // or we need a cleaner accumulater.
            // Let's assume we pass a buffer in user_data.
            // Actually, simplified approach: use esp_http_client_read_response in the main flow
            // or accumulate here.
            
            // Standard way: accumulate in a dynamic buffer
            char **output_buffer = (char **)evt->user_data;
            if (*output_buffer == NULL) {
                *output_buffer = (char *)malloc(evt->data_len + 1);
                memcpy(*output_buffer, evt->data, evt->data_len);
                (*output_buffer)[evt->data_len] = 0;
            } else {
                int current_len = strlen(*output_buffer);
                *output_buffer = (char *)realloc(*output_buffer, current_len + evt->data_len + 1);
                memcpy(*output_buffer + current_len, evt->data, evt->data_len);
                (*output_buffer)[current_len + evt->data_len] = 0;
            }
        }
    }
    return ESP_OK;
}

char *kb_chat_query(const char *text) {
    char *response_buffer = NULL;
    char *answer = NULL;
    
    // 1. Prepare Payload
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "query", text);
    char *post_data = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (!post_data) {
        ESP_LOGE(TAG, "Failed to create JSON payload");
        return NULL;
    }

    // 2. Configure Client
    esp_http_client_config_t config = {
        .url = sys_param->kb_url,
        .event_handler = _http_event_handle,
        .user_data = &response_buffer,
        .buffer_size = 2048, /* Rx buffer size */
        .timeout_ms = 10000,
        .disable_auto_redirect = true, // We likely don't need redirect for API
        .crt_bundle_attach = esp_crt_bundle_attach,
        //.skip_cert_common_name_check = true, // Uncomment if needed, but bundle attach should be enough for public
    };
    
    // If sys_param->kb_url is empty, it will fail.
    if (strlen(sys_param->kb_url) == 0) {
        ESP_LOGE(TAG, "KB URL is missing");
        free(post_data);
        return NULL;
    }

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    // 3. Perform Request
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        if (status_code == 200 && response_buffer != NULL) {
             ESP_LOGI(TAG, "KB Response: %s", response_buffer);
             // 4. Parse Response
             cJSON *json = cJSON_Parse(response_buffer);
             if (json) {
                 cJSON *answer_item = cJSON_GetObjectItemCaseSensitive(json, "answer");
                 if (cJSON_IsString(answer_item) && (answer_item->valuestring != NULL)) {
                     answer = strdup(answer_item->valuestring);
                 }
                 cJSON_Delete(json);
             }
        } else {
             ESP_LOGE(TAG, "KB Request failed status: %d", status_code);
        }
    } else {
        ESP_LOGE(TAG, "KB Request failed: %s", esp_err_to_name(err));
    }

    // Cleanup
    esp_http_client_cleanup(client);
    free(post_data);
    if (response_buffer) {
        free(response_buffer);
    }

    return answer;
}

/* program flow. This function is called in app_audio.c */
esp_err_t start_openai(uint8_t *audio, int audio_len)
{
    esp_err_t ret = ESP_OK;
    static OpenAI_t *openai = NULL;
    static OpenAI_AudioTranscription_t *audioTranscription = NULL;
    static OpenAI_ChatCompletion_t *chatCompletion = NULL;
    static OpenAI_AudioSpeech_t *audioSpeech = NULL;

    OpenAI_SpeechResponse_t *speechresult = NULL;
    OpenAI_StringResponse_t *result = NULL;
    FILE *fp = NULL;
    char *response = NULL;

    if (openai == NULL) {
        openai = OpenAICreate(sys_param->key);
        ESP_RETURN_ON_FALSE(NULL != openai, ESP_ERR_INVALID_ARG, TAG, "OpenAICreate faield");

        OpenAIChangeBaseURL(openai, sys_param->url);

        audioTranscription = openai->audioTranscriptionCreate(openai);
        chatCompletion = openai->chatCreate(openai);
        audioSpeech = openai->audioSpeechCreate(openai);

        audioTranscription->setResponseFormat(audioTranscription, OPENAI_AUDIO_RESPONSE_FORMAT_JSON);
        audioTranscription->setLanguage(audioTranscription, "en");
        audioTranscription->setTemperature(audioTranscription, 0.2);

        chatCompletion->setModel(chatCompletion, "gpt-3.5-turbo");
        chatCompletion->setSystem(chatCompletion, "user");
        chatCompletion->setMaxTokens(chatCompletion, CONFIG_MAX_TOKEN);
        chatCompletion->setTemperature(chatCompletion, 0.2);
        chatCompletion->setStop(chatCompletion, "\r");
        chatCompletion->setPresencePenalty(chatCompletion, 0);
        chatCompletion->setFrequencyPenalty(chatCompletion, 0);
        chatCompletion->setUser(chatCompletion, "OpenAI-ESP32");

        audioSpeech->setModel(audioSpeech, "tts-1");
        // TTS voice from NVS settings
        // Available voices:
        // Male voices: alloy, echo, onyx
        // Female voices: fable, nova, shimmer
        // Use voice from settings (defaults to "shimmer" if not configured)
        audioSpeech->setVoice(audioSpeech, sys_param->tts_voice);
        audioSpeech->setResponseFormat(audioSpeech, OPENAI_AUDIO_OUTPUT_FORMAT_MP3);
        audioSpeech->setSpeed(audioSpeech, 1.0);
    }

    ui_ctrl_show_panel(UI_CTRL_PANEL_GET, 0);

    // OpenAI Audio Transcription
    char *text = audioTranscription->file(audioTranscription, (uint8_t *)audio, audio_len, OPENAI_AUDIO_INPUT_FORMAT_WAV);

    if (NULL == text) {
        ret = ESP_ERR_INVALID_RESPONSE;
        ui_ctrl_label_show_text(UI_CTRL_LABEL_LISTEN_SPEAK, INVALID_REQUEST_ERROR);
        ui_ctrl_show_panel(UI_CTRL_PANEL_SLEEP, LISTEN_SPEAK_PANEL_DELAY_MS);
        ESP_GOTO_ON_ERROR(ret, err, TAG, "[audioTranscription]: invalid url");
    }

    if (strstr(text, "\"code\": ")) {
        ret = ESP_ERR_INVALID_RESPONSE;
        ui_ctrl_label_show_text(UI_CTRL_LABEL_LISTEN_SPEAK, text);
        ui_ctrl_show_panel(UI_CTRL_PANEL_SLEEP, LISTEN_SPEAK_PANEL_DELAY_MS);
        ESP_GOTO_ON_ERROR(ret, err, TAG, "[audioTranscription]: invalid response");
    }

    if (strcmp(text, INVALID_REQUEST_ERROR) == 0 || strcmp(text, SERVER_ERROR) == 0) {
        ret = ESP_ERR_INVALID_RESPONSE;
        ui_ctrl_label_show_text(UI_CTRL_LABEL_LISTEN_SPEAK, SORRY_CANNOT_UNDERSTAND);
        ui_ctrl_show_panel(UI_CTRL_PANEL_SLEEP, LISTEN_SPEAK_PANEL_DELAY_MS);
        ESP_GOTO_ON_ERROR(ret, err, TAG, "[audioTranscription]: invalid response");
    }

    // UI listen success
    ui_ctrl_label_show_text(UI_CTRL_LABEL_REPLY_QUESTION, text);
    ui_ctrl_label_show_text(UI_CTRL_LABEL_LISTEN_SPEAK, text);

    // KB Chat Query
    response = kb_chat_query(text);
    if (NULL == response) {
        ret = ESP_ERR_INVALID_RESPONSE;
        ui_ctrl_label_show_text(UI_CTRL_LABEL_LISTEN_SPEAK, SORRY_CANNOT_UNDERSTAND);
        ui_ctrl_show_panel(UI_CTRL_PANEL_SLEEP, LISTEN_SPEAK_PANEL_DELAY_MS);
        ESP_GOTO_ON_ERROR(ret, err, TAG, "[kb_chat_query]: invalid response");
    }

    if (response != NULL && (strcmp(response, INVALID_REQUEST_ERROR) == 0 || strcmp(response, SERVER_ERROR) == 0)) {
        // UI listen fail
        ret = ESP_ERR_INVALID_RESPONSE;
        ui_ctrl_label_show_text(UI_CTRL_LABEL_LISTEN_SPEAK, SORRY_CANNOT_UNDERSTAND);
        ui_ctrl_show_panel(UI_CTRL_PANEL_SLEEP, LISTEN_SPEAK_PANEL_DELAY_MS);
        ESP_GOTO_ON_ERROR(ret, err, TAG, "[chatCompletion]: invalid response");
    }

    // UI listen success
    ui_ctrl_label_show_text(UI_CTRL_LABEL_REPLY_QUESTION, text);
    // ui_ctrl_label_show_text(UI_CTRL_LABEL_LISTEN_SPEAK, response);
    // Removed API response text from LISTEN_SPEAK label - only show STT text, not API response

    if (strcmp(response, INVALID_REQUEST_ERROR) == 0) {
        ret = ESP_ERR_INVALID_RESPONSE;
        ui_ctrl_label_show_text(UI_CTRL_LABEL_LISTEN_SPEAK, SORRY_CANNOT_UNDERSTAND);
        ui_ctrl_show_panel(UI_CTRL_PANEL_SLEEP, LISTEN_SPEAK_PANEL_DELAY_MS);
        ESP_GOTO_ON_ERROR(ret, err, TAG, "[chatCompletion]: invalid response");
    }

    ui_ctrl_label_show_text(UI_CTRL_LABEL_REPLY_CONTENT, response);
    ui_ctrl_show_panel(UI_CTRL_PANEL_REPLY, 0);

    // OpenAI Speech Response
    speechresult = audioSpeech->speech(audioSpeech, response);
    if (NULL == speechresult) {
        ret = ESP_ERR_INVALID_RESPONSE;
        ui_ctrl_show_panel(UI_CTRL_PANEL_SLEEP, 5 * LISTEN_SPEAK_PANEL_DELAY_MS);
        fp = fopen("/spiffs/tts_failed.mp3", "r");
        if (fp) {
            audio_player_play(fp);
        }
        ESP_GOTO_ON_ERROR(ret, err, TAG, "[audioSpeech]: invalid response");
    }

    uint32_t dataLength = speechresult->getLen(speechresult);
    char *speechptr = speechresult->getData(speechresult);
    esp_err_t status = ESP_FAIL;
    fp = fmemopen((void *)speechptr, dataLength, "rb");
    if (fp) {
        // Start subtitle system before playing audio
        // Subtitles will be triggered when audio actually starts playing (via callback)
        ui_ctrl_subtitle_start(response);
        status = audio_player_play(fp);
    }

    if (status != ESP_OK) {
        ESP_LOGE(TAG, "Error creating ChatGPT request: %s\n", esp_err_to_name(status));
        // UI reply audio fail
        ui_ctrl_show_panel(UI_CTRL_PANEL_SLEEP, 0);
    }
    // Note: Speaking animation is now triggered automatically in audio_player_cb 
    // when AUDIO_PLAYER_CALLBACK_EVENT_PLAYING event fires

err:
    // Clearing resources
    if (speechresult) {
        speechresult->deleteResponse (speechresult);
    }

    if (result) {
        result->deleteResponse (result);
    }

    if (text) {
        free(text);
    }
    if (response) {
        free(response);
    }
    return ret;
}

/* play audio function */

static void audio_play_finish_cb(void)
{
    ESP_LOGI(TAG, "replay audio end");
    if (ui_ctrl_reply_get_audio_start_flag()) {
        ui_ctrl_reply_set_audio_end_flag(true);
    }
}

void app_main()
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(settings_read_parameter_from_nvs());
    sys_param = settings_get_parameter();

    bsp_spiffs_mount();
    bsp_i2c_init();

    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size = BSP_LCD_H_RES * CONFIG_BSP_LCD_DRAW_BUF_HEIGHT,
        .double_buffer = 0,
        .flags = {
            .buff_dma = true,
        }
    };
    bsp_display_start_with_config(&cfg);
    bsp_board_init();

    ESP_LOGI(TAG, "Display LVGL demo");
    bsp_display_backlight_on();
    ui_ctrl_init();
    
    // Apply theme from settings
    ESP_LOGI(TAG, "Applying theme from settings");
    app_theme_apply(sys_param);
    
    app_network_start();

    ESP_LOGI(TAG, "speech recognition start");
    app_sr_start(false);
    audio_register_play_finish_cb(audio_play_finish_cb);

    while (true) {

        ESP_LOGD(TAG, "\tDescription\tInternal\tSPIRAM");
        ESP_LOGD(TAG, "Current Free Memory\t%d\t\t%d",
                 heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL),
                 heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
        ESP_LOGD(TAG, "Min. Ever Free Size\t%d\t\t%d",
                 heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL),
                 heap_caps_get_minimum_free_size(MALLOC_CAP_SPIRAM));
        vTaskDelay(pdMS_TO_TICKS(5 * 1000));
    }
}
