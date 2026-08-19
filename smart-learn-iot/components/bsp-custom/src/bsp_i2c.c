#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "bsp_board.h"

static const char *TAG = "bsp_i2c";

static i2c_master_bus_handle_t s_i2c_bus;

i2c_master_bus_handle_t bsp_i2c_get_bus(void)
{
    return s_i2c_bus;
}

esp_err_t bsp_i2c_init(void)
{
    if (s_i2c_bus) {
        return ESP_OK;
    }

    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = CONFIG_BOARD_I2C_SDA_GPIO,
        .scl_io_num = CONFIG_BOARD_I2C_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    esp_err_t ret = i2c_new_master_bus(&bus_cfg, &s_i2c_bus);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "I2C bus ready (SDA=%d, SCL=%d)", CONFIG_BOARD_I2C_SDA_GPIO, CONFIG_BOARD_I2C_SCL_GPIO);
    }
    return ret;
}
