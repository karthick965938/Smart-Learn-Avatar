#include "esp_log.h"
#include "esp_check.h"
#include "bsp_board.h"
#include "bsp_oled.h"
#include "bsp_rfid.h"

static const char *TAG = "bsp_board";

esp_err_t bsp_board_init(void)
{
    ESP_RETURN_ON_ERROR(bsp_codec_set_fs(16000, 16, I2S_SLOT_MODE_MONO), TAG, "audio init failed");
    ESP_RETURN_ON_ERROR(bsp_i2c_init(), TAG, "I2C init failed");
    ESP_RETURN_ON_ERROR(bsp_oled_init(), TAG, "OLED init failed");
    ESP_RETURN_ON_ERROR(bsp_rfid_init(), TAG, "RFID init failed");
    ESP_LOGI(TAG, "Custom ESP32-S3 board initialized");
    return ESP_OK;
}
