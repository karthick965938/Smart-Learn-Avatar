#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/i2s_std.h"
#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BSP_SPIFFS_MOUNT_POINT "/spiffs"

esp_err_t bsp_board_init(void);
esp_err_t bsp_spiffs_mount(void);
esp_err_t bsp_i2c_init(void);

i2c_master_bus_handle_t bsp_i2c_get_bus(void);

esp_err_t bsp_i2s_read(void *audio_buffer, size_t len, size_t *bytes_read, uint32_t timeout_ms);
esp_err_t bsp_i2s_write(void *audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms);
esp_err_t bsp_codec_set_fs(uint32_t rate, uint32_t bits_cfg, i2s_slot_mode_t ch);
esp_err_t bsp_codec_volume_set(int volume, int *volume_set);
esp_err_t bsp_codec_mute_set(bool enable);
esp_err_t bsp_codec_dev_stop(void);
esp_err_t bsp_codec_dev_resume(void);

#ifdef __cplusplus
}
#endif
