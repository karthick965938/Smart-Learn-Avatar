#include <stdio.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "app_sr.h"
#include "app_audio.h"
#include "app_ui_ctrl.h"
#include "bsp_board.h"
#include "audio_player.h"
#include "file_iterator.h"
#include "app_wifi.h"

static const char *TAG = "app_audio";

bool record_flag = false;
uint32_t record_total_len = 0;
uint32_t file_total_len = 0;
static uint8_t *record_audio_buffer = NULL;
uint8_t *audio_rx_buffer = NULL;
static audio_play_finish_cb_t audio_play_finish_cb = NULL;

extern sr_data_t *g_sr_data;
extern esp_err_t start_openai(uint8_t *audio, int audio_len);
extern int Cache_WriteBack_Addr(uint32_t addr, uint32_t size);

static esp_err_t audio_mute_function(AUDIO_PLAYER_MUTE_SETTING setting)
{
    bsp_codec_mute_set(setting == AUDIO_PLAYER_MUTE);
    if (setting == AUDIO_PLAYER_UNMUTE) {
        bsp_codec_volume_set(CONFIG_VOLUME_LEVEL, NULL);
    }
    return ESP_OK;
}

static esp_err_t audio_codec_set_fs(uint32_t rate, uint32_t bits_cfg, i2s_slot_mode_t ch)
{
    esp_err_t ret = bsp_codec_set_fs(rate, bits_cfg, ch);
    bsp_codec_mute_set(true);
    bsp_codec_mute_set(false);
    bsp_codec_volume_set(CONFIG_VOLUME_LEVEL, NULL);
    vTaskDelay(pdMS_TO_TICKS(20));
    return ret;
}

static void audio_player_cb(audio_player_cb_ctx_t *ctx)
{
    switch (ctx->audio_event) {
    case AUDIO_PLAYER_CALLBACK_EVENT_IDLE:
        bsp_codec_set_fs(16000, 16, I2S_SLOT_MODE_MONO);
        if (audio_play_finish_cb) {
            audio_play_finish_cb();
        }
        ui_ctrl_reply_set_audio_end_flag(true);
        break;
    case AUDIO_PLAYER_CALLBACK_EVENT_PLAYING:
        ui_ctrl_reply_set_audio_start_flag(true);
        break;
    default:
        break;
    }
}

void audio_record_init(void)
{
#if DEBUG_SAVE_PCM
    record_audio_buffer = heap_caps_calloc(1, FILE_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    audio_rx_buffer = heap_caps_calloc(1, MAX_FILE_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
#endif

    if (!record_audio_buffer || !audio_rx_buffer) {
        ESP_LOGE(TAG, "Failed to allocate audio buffers");
        return;
    }

    file_iterator_instance_t *file_iterator = file_iterator_new(BSP_SPIFFS_MOUNT_POINT);
    assert(file_iterator != NULL);

    audio_player_config_t config = {
        .mute_fn = audio_mute_function,
        .write_fn = bsp_i2s_write,
        .clk_set_fn = audio_codec_set_fs,
        .priority = 5,
    };
    ESP_ERROR_CHECK(audio_player_new(config));
    audio_player_callback_register(audio_player_cb, NULL);
}

void audio_record_save(int16_t *audio_buffer, int audio_chunksize)
{
#if DEBUG_SAVE_PCM
    if (record_flag) {
        uint16_t *record_buff = (uint16_t *)(record_audio_buffer + sizeof(wav_header_t));
        record_buff += record_total_len;
        for (int i = 0; i < (audio_chunksize - 1); i++) {
            if (record_total_len < (MAX_FILE_SIZE - sizeof(wav_header_t)) / 2) {
                record_buff[i] = audio_buffer[i * 3];
                record_total_len += 1;
            }
        }
    }
#endif
}

void audio_register_play_finish_cb(audio_play_finish_cb_t cb)
{
    audio_play_finish_cb = cb;
}

static void audio_record_start(void)
{
#if DEBUG_SAVE_PCM
    audio_player_stop();
    record_flag = true;
    record_total_len = 0;
    file_total_len = sizeof(wav_header_t);
#endif
}

static esp_err_t audio_record_stop(void)
{
#if DEBUG_SAVE_PCM
    record_flag = false;
    file_total_len += record_total_len;
    ESP_LOGI(TAG, "record stop, %" PRIu32 " bytes", record_total_len);

    FILE *fp = fopen("/spiffs/echo_en_wake.wav", "r");
    if (!fp) {
        return ESP_FAIL;
    }

    wav_header_t wav_head;
    if (fread(&wav_head, 1, sizeof(wav_header_t), fp) <= 0) {
        fclose(fp);
        return ESP_FAIL;
    }

    wav_head.SampleRate = 16000;
    wav_head.NumChannels = 1;
    wav_head.BitsPerSample = 16;
    wav_head.ChunkSize = file_total_len - 8;
    wav_head.ByteRate = wav_head.SampleRate * wav_head.BitsPerSample * wav_head.NumChannels / 8;
    wav_head.Subchunk2ID[0] = 'd';
    wav_head.Subchunk2ID[1] = 'a';
    wav_head.Subchunk2ID[2] = 't';
    wav_head.Subchunk2ID[3] = 'a';
    wav_head.Subchunk2Size = record_total_len;
    memcpy(record_audio_buffer, &wav_head, sizeof(wav_header_t));
    Cache_WriteBack_Addr((uint32_t)record_audio_buffer, record_total_len);
    fclose(fp);
#endif
    return ESP_OK;
}

esp_err_t audio_play_task(void *filepath)
{
    FILE *fp = NULL;
    struct stat file_stat;
    esp_err_t ret = ESP_OK;
    const size_t chunk_size = 4096;
    uint8_t *buffer = malloc(chunk_size);

    if (!buffer) {
        return ESP_FAIL;
    }
    if (stat(filepath, &file_stat) != 0) {
        free(buffer);
        return ESP_FAIL;
    }

    fp = fopen(filepath, "r");
    if (!fp) {
        free(buffer);
        return ESP_FAIL;
    }

    wav_header_t wav_head;
    if (fread(&wav_head, 1, sizeof(wav_header_t), fp) <= 0) {
        ret = ESP_FAIL;
        goto exit;
    }

    if (!strstr((char *)wav_head.Subchunk1ID, "fmt") && !strstr((char *)wav_head.Subchunk2ID, "data")) {
        fseek(fp, 0, SEEK_SET);
        wav_head.SampleRate = 16000;
        wav_head.NumChannels = 1;
        wav_head.BitsPerSample = 16;
    }

    bsp_codec_set_fs(wav_head.SampleRate, wav_head.BitsPerSample, I2S_SLOT_MODE_MONO);
    bsp_codec_mute_set(false);
    bsp_codec_volume_set(CONFIG_VOLUME_LEVEL, NULL);

    size_t cnt;
    int len;
    do {
        len = fread(buffer, 1, chunk_size, fp);
        if (len <= 0) {
            break;
        }
        bsp_i2s_write(buffer, len, &cnt, portMAX_DELAY);
    } while (1);

exit:
    if (fp) {
        fclose(fp);
    }
    free(buffer);
    return ret;
}

void sr_handler_task(void *pvParam)
{
    (void)pvParam;
    while (true) {
        if (NEED_DELETE && xEventGroupGetBits(g_sr_data->event_group)) {
            xEventGroupSetBits(g_sr_data->event_group, HANDLE_DELETED);
            vTaskDelete(NULL);
        }

        sr_result_t result = {
            .wakenet_mode = WAKENET_NO_DETECT,
            .state = ESP_MN_STATE_DETECTING,
        };

        app_sr_get_result(&result, pdMS_TO_TICKS(500));

        if (ESP_MN_STATE_TIMEOUT == result.state) {
            audio_record_stop();
            if (WIFI_STATUS_CONNECTED_OK == wifi_connected_already()) {
                start_openai((uint8_t *)record_audio_buffer, record_total_len + sizeof(wav_header_t));
            }
            continue;
        }

        if (WAKENET_DETECTED == result.wakenet_mode) {
            audio_record_start();
            ui_ctrl_guide_jump();
            ui_ctrl_show_panel(UI_CTRL_PANEL_LISTEN, 0);
            audio_play_task("/spiffs/echo_en_wake.wav");
            continue;
        }

        if (ESP_MN_STATE_DETECTED & result.state) {
            audio_record_stop();
            audio_play_task("/spiffs/echo_en_ok.wav");
            continue;
        }
    }
}
