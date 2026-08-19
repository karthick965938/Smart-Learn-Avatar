#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_check.h"
#include "nvs_flash.h"
#include "app_ui_ctrl.h"
#include "OpenAI.h"
#include "audio_player.h"
#include "app_sr.h"
#include "bsp_board.h"
#include "app_audio.h"
#include "app_wifi.h"
#include "app_rfid.h"
#include "settings.h"
#include "esp_http_client.h"
#include "cJSON.h"
#include "esp_crt_bundle.h"

#define LISTEN_SPEAK_PANEL_DELAY_MS 2000
#define SERVER_ERROR "server_error"
#define INVALID_REQUEST_ERROR "invalid_request_error"

#define MSG_STT_FAILED "Speech recognition failed."
#define MSG_STT_ERROR "Transcription error."
#define MSG_KB_FAILED "Knowledge base error."

static char *TAG = "app_main";
static sys_param_t *sys_param = NULL;

esp_err_t _http_event_handle(esp_http_client_event_t *evt)
{
    if (evt->event_id == HTTP_EVENT_ON_DATA) {
        if (!esp_http_client_is_chunked_response(evt->client)) {
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

char *kb_chat_query(const char *text)
{
    char *response_buffer = NULL;
    char *answer = NULL;
    const char *kb_url = settings_get_active_kb_url();

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "query", text);
    char *post_data = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (!post_data || !kb_url || strlen(kb_url) == 0) {
        free(post_data);
        return NULL;
    }

    esp_http_client_config_t config = {
        .url = kb_url,
        .event_handler = _http_event_handle,
        .user_data = &response_buffer,
        .buffer_size = 2048,
        .timeout_ms = 10000,
        .disable_auto_redirect = true,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        if (status_code == 200 && response_buffer != NULL) {
            cJSON *json = cJSON_Parse(response_buffer);
            if (json) {
                cJSON *answer_item = cJSON_GetObjectItemCaseSensitive(json, "answer");
                if (cJSON_IsString(answer_item) && answer_item->valuestring != NULL) {
                    answer = strdup(answer_item->valuestring);
                }
                cJSON_Delete(json);
            }
        }
    }

    esp_http_client_cleanup(client);
    free(post_data);
    free(response_buffer);
    return answer;
}

esp_err_t start_openai(uint8_t *audio, int audio_len)
{
    esp_err_t ret = ESP_OK;
    static OpenAI_t *openai = NULL;
    static OpenAI_AudioTranscription_t *audioTranscription = NULL;
    static OpenAI_AudioSpeech_t *audioSpeech = NULL;

    OpenAI_SpeechResponse_t *speechresult = NULL;
    FILE *fp = NULL;
    char *response = NULL;

    if (openai == NULL) {
        openai = OpenAICreate(sys_param->key);
        ESP_RETURN_ON_FALSE(openai != NULL, ESP_ERR_INVALID_ARG, TAG, "OpenAICreate failed");
        OpenAIChangeBaseURL(openai, sys_param->url);

        audioTranscription = openai->audioTranscriptionCreate(openai);
        audioSpeech = openai->audioSpeechCreate(openai);

        audioTranscription->setResponseFormat(audioTranscription, OPENAI_AUDIO_RESPONSE_FORMAT_JSON);
        audioTranscription->setLanguage(audioTranscription, "en");
        audioTranscription->setTemperature(audioTranscription, 0.2);

        audioSpeech->setModel(audioSpeech, "tts-1");
        audioSpeech->setVoice(audioSpeech, sys_param->tts_voice);
        audioSpeech->setResponseFormat(audioSpeech, OPENAI_AUDIO_OUTPUT_FORMAT_MP3);
        audioSpeech->setSpeed(audioSpeech, 1.0);
    }

    ui_ctrl_show_panel(UI_CTRL_PANEL_GET, 0);

    char *text = audioTranscription->file(audioTranscription, audio, audio_len, OPENAI_AUDIO_INPUT_FORMAT_WAV);
    if (text == NULL) {
        ui_ctrl_label_show_text(UI_CTRL_LABEL_LISTEN_SPEAK, MSG_STT_FAILED);
        ui_ctrl_show_panel(UI_CTRL_PANEL_SLEEP, LISTEN_SPEAK_PANEL_DELAY_MS);
        return ESP_ERR_INVALID_RESPONSE;
    }

    if (strstr(text, "\"code\": ") || strcmp(text, INVALID_REQUEST_ERROR) == 0 || strcmp(text, SERVER_ERROR) == 0) {
        ui_ctrl_label_show_text(UI_CTRL_LABEL_LISTEN_SPEAK, MSG_STT_ERROR);
        ui_ctrl_show_panel(UI_CTRL_PANEL_SLEEP, LISTEN_SPEAK_PANEL_DELAY_MS);
        free(text);
        return ESP_ERR_INVALID_RESPONSE;
    }

    ui_ctrl_label_show_text(UI_CTRL_LABEL_REPLY_QUESTION, text);
    ui_ctrl_label_show_text(UI_CTRL_LABEL_LISTEN_SPEAK, text);

    response = kb_chat_query(text);
    if (response == NULL) {
        ui_ctrl_label_show_text(UI_CTRL_LABEL_LISTEN_SPEAK, MSG_KB_FAILED);
        ui_ctrl_show_panel(UI_CTRL_PANEL_SLEEP, LISTEN_SPEAK_PANEL_DELAY_MS);
        free(text);
        return ESP_ERR_INVALID_RESPONSE;
    }

    ui_ctrl_label_show_text(UI_CTRL_LABEL_REPLY_CONTENT, response);
    ui_ctrl_show_panel(UI_CTRL_PANEL_REPLY, 0);

    speechresult = audioSpeech->speech(audioSpeech, response);
    if (speechresult == NULL) {
        fp = fopen("/spiffs/tts_failed.mp3", "r");
        if (fp) {
            audio_player_play(fp);
        }
        free(text);
        free(response);
        return ESP_ERR_INVALID_RESPONSE;
    }

    uint32_t dataLength = speechresult->getLen(speechresult);
    char *speechptr = speechresult->getData(speechresult);
    fp = fmemopen((void *)speechptr, dataLength, "rb");
    if (fp) {
        ui_ctrl_subtitle_start(response);
        audio_player_play(fp);
    }

    if (speechresult) {
        speechresult->deleteResponse(speechresult);
    }
    free(text);
    free(response);
    return ret;
}

static void audio_play_finish_cb(void)
{
    if (ui_ctrl_reply_get_audio_start_flag()) {
        ui_ctrl_reply_set_audio_end_flag(true);
    }
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(settings_read_parameter_from_nvs());
    sys_param = settings_get_parameter();

    ESP_ERROR_CHECK(bsp_spiffs_mount());
    ESP_ERROR_CHECK(bsp_board_init());
    ui_ctrl_init();

    app_network_start();
    app_rfid_start();

    ESP_LOGI(TAG, "Speech recognition starting");
    app_sr_start(false);
    audio_register_play_finish_cb(audio_play_finish_cb);

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
