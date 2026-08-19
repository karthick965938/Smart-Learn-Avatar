#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_check.h"
#include "sdkconfig.h"
#include "bsp_board.h"

static const char *TAG = "bsp_i2s";

static i2s_chan_handle_t s_rx_chan;
static i2s_chan_handle_t s_tx_chan;
static bool s_muted;
static int s_volume = 90;

static esp_err_t i2s_mic_init(uint32_t rate)
{
    if (s_rx_chan) {
        i2s_channel_disable(s_rx_chan);
        i2s_del_channel(s_rx_chan);
        s_rx_chan = NULL;
    }

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    ESP_RETURN_ON_ERROR(i2s_new_channel(&chan_cfg, NULL, &s_rx_chan), TAG, "mic channel");

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(rate),
        .slot_cfg = I2S_STD_PHILIP_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = CONFIG_BOARD_I2S_MIC_BCLK_GPIO,
            .ws = CONFIG_BOARD_I2S_MIC_WS_GPIO,
            .dout = I2S_GPIO_UNUSED,
            .din = CONFIG_BOARD_I2S_MIC_DIN_GPIO,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(s_rx_chan, &std_cfg), TAG, "mic std");
    return i2s_channel_enable(s_rx_chan);
}

static esp_err_t i2s_spk_init(uint32_t rate, uint32_t bits, i2s_slot_mode_t ch)
{
    if (s_tx_chan) {
        i2s_channel_disable(s_tx_chan);
        i2s_del_channel(s_tx_chan);
        s_tx_chan = NULL;
    }

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_1, I2S_ROLE_MASTER);
    ESP_RETURN_ON_ERROR(i2s_new_channel(&chan_cfg, &s_tx_chan, NULL), TAG, "spk channel");

    i2s_data_bit_width_t bit_width = (bits == 32) ? I2S_DATA_BIT_WIDTH_32BIT : I2S_DATA_BIT_WIDTH_16BIT;
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(rate),
        .slot_cfg = I2S_STD_PHILIP_SLOT_DEFAULT_CONFIG(bit_width, ch),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = CONFIG_BOARD_I2S_SPK_BCLK_GPIO,
            .ws = CONFIG_BOARD_I2S_SPK_WS_GPIO,
            .dout = CONFIG_BOARD_I2S_SPK_DOUT_GPIO,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(s_tx_chan, &std_cfg), TAG, "spk std");
    return i2s_channel_enable(s_tx_chan);
}

esp_err_t bsp_codec_set_fs(uint32_t rate, uint32_t bits_cfg, i2s_slot_mode_t ch)
{
    esp_err_t ret = i2s_mic_init(rate);
    if (ret != ESP_OK) {
        return ret;
    }
    return i2s_spk_init(rate, bits_cfg, ch);
}

esp_err_t bsp_codec_volume_set(int volume, int *volume_set)
{
    if (volume < 0) {
        volume = 0;
    } else if (volume > 100) {
        volume = 100;
    }
    s_volume = volume;
    if (volume_set) {
        *volume_set = s_volume;
    }
    return ESP_OK;
}

esp_err_t bsp_codec_mute_set(bool enable)
{
    s_muted = enable;
    return ESP_OK;
}

esp_err_t bsp_codec_dev_stop(void)
{
    if (s_tx_chan) {
        i2s_channel_disable(s_tx_chan);
    }
    if (s_rx_chan) {
        i2s_channel_disable(s_rx_chan);
    }
    return ESP_OK;
}

esp_err_t bsp_codec_dev_resume(void)
{
    return bsp_codec_set_fs(16000, 16, I2S_SLOT_MODE_MONO);
}

esp_err_t bsp_i2s_read(void *audio_buffer, size_t len, size_t *bytes_read, uint32_t timeout_ms)
{
    if (!s_rx_chan) {
        ESP_RETURN_ON_ERROR(bsp_codec_set_fs(16000, 16, I2S_SLOT_MODE_MONO), TAG, "init mic");
    }

    size_t mono_bytes = len / 2;
    int32_t *raw = heap_caps_malloc(mono_bytes * 2, MALLOC_CAP_INTERNAL);
    if (!raw) {
        return ESP_ERR_NO_MEM;
    }

    size_t read_bytes = 0;
    esp_err_t ret = i2s_channel_read(s_rx_chan, raw, mono_bytes * 2, &read_bytes, pdMS_TO_TICKS(timeout_ms));
    if (ret != ESP_OK) {
        free(raw);
        return ret;
    }

    int16_t *out = audio_buffer;
    size_t samples = read_bytes / sizeof(int32_t);
    for (size_t i = 0; i < samples; i++) {
        int32_t sample = raw[i] >> 14;
        if (sample > 32767) {
            sample = 32767;
        } else if (sample < -32768) {
            sample = -32768;
        }
        int16_t s16 = (int16_t)sample;
        out[i * 2] = s16;
        out[i * 2 + 1] = s16;
    }

    if (bytes_read) {
        *bytes_read = samples * sizeof(int16_t) * 2;
    }
    free(raw);
    return ESP_OK;
}

esp_err_t bsp_i2s_write(void *audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms)
{
    if (!s_tx_chan) {
        ESP_RETURN_ON_ERROR(bsp_codec_set_fs(16000, 16, I2S_SLOT_MODE_MONO), TAG, "init spk");
    }

    if (s_muted) {
        if (bytes_written) {
            *bytes_written = len;
        }
        return ESP_OK;
    }

    int16_t *in = audio_buffer;
    size_t stereo_samples = len / sizeof(int16_t);
    size_t mono_samples = stereo_samples / 2;
    int16_t *mono = heap_caps_malloc(mono_samples * sizeof(int16_t), MALLOC_CAP_INTERNAL);
    if (!mono) {
        return ESP_ERR_NO_MEM;
    }

    float gain = s_volume / 100.0f;
    for (size_t i = 0; i < mono_samples; i++) {
        int32_t mixed = ((int32_t)in[i * 2] + (int32_t)in[i * 2 + 1]) / 2;
        mixed = (int32_t)(mixed * gain);
        if (mixed > 32767) {
            mixed = 32767;
        } else if (mixed < -32768) {
            mixed = -32768;
        }
        mono[i] = (int16_t)mixed;
    }

    size_t write_len = mono_samples * sizeof(int16_t);
    size_t written = 0;
    esp_err_t ret = i2s_channel_write(s_tx_chan, mono, write_len, &written, pdMS_TO_TICKS(timeout_ms));
    free(mono);

    if (bytes_written) {
        *bytes_written = len;
    }
    return ret;
}
