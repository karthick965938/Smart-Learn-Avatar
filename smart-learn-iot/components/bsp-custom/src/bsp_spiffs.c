#include <string.h>
#include "esp_log.h"
#include "esp_spiffs.h"
#include "bsp_board.h"

static const char *TAG = "bsp_spiffs";

esp_err_t bsp_spiffs_mount(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = BSP_SPIFFS_MOUNT_POINT,
        .partition_label = "storage",
        .max_files = 5,
        .format_if_mount_failed = false,
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format SPIFFS");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "SPIFFS partition not found");
        }
        return ret;
    }

    size_t total = 0;
    size_t used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "SPIFFS: total=%u, used=%u", (unsigned)total, (unsigned)used);
    }
    return ESP_OK;
}
