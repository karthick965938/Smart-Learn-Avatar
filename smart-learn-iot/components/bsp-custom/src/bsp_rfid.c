#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "esp_check.h"
#include "sdkconfig.h"
#include "bsp_rfid.h"

static const char *TAG = "bsp_rfid";

#define MFRC522_CMD_IDLE       0x00
#define MFRC522_CMD_MEM        0x01
#define MFRC522_CMD_GENERATEID 0x02
#define MFRC522_CMD_CALCCRC    0x03
#define MFRC522_CMD_TRANSMIT   0x04
#define MFRC522_CMD_NOCMDCHANGE 0x07
#define MFRC522_CMD_RECEIVE    0x08
#define MFRC522_CMD_TRANSCEIVE 0x0C
#define MFRC522_CMD_MFAUTHENT  0x0E
#define MFRC522_CMD_SOFTRESET  0x0F

#define MFRC522_REG_COMMAND    0x01
#define MFRC522_REG_COMIRQ     0x04
#define MFRC522_REG_DIVIRQ     0x05
#define MFRC522_REG_ERROR      0x06
#define MFRC522_REG_STATUS2    0x08
#define MFRC522_REG_FIFO_DATA  0x09
#define MFRC522_REG_FIFO_LEVEL 0x0A
#define MFRC522_REG_CONTROL    0x0C
#define MFRC522_REG_BIT_FRAMING 0x0D
#define MFRC522_REG_MODE       0x11
#define MFRC522_REG_TX_CONTROL 0x14
#define MFRC522_REG_TX_AUTO    0x15
#define MFRC522_REG_VERSION    0x37

#define MFRC522_PICC_REQIDL    0x26
#define MFRC522_PICC_ANTICOLL  0x93
#define MFRC522_PICC_HALT      0x50

static spi_device_handle_t s_spi;
static bsp_rfid_card_cb_t s_card_cb;
static void *s_card_cb_arg;
static char s_last_uid[16];

static void rfid_write_reg(uint8_t reg, uint8_t value)
{
    uint8_t tx[2] = { (uint8_t)((reg << 1) & 0x7E), value };
    spi_transaction_t t = {
        .length = 16,
        .tx_buffer = tx,
    };
    gpio_set_level(CONFIG_BOARD_RFID_CS_GPIO, 0);
    spi_device_transmit(s_spi, &t);
    gpio_set_level(CONFIG_BOARD_RFID_CS_GPIO, 1);
}

static uint8_t rfid_read_reg(uint8_t reg)
{
    uint8_t tx[2] = { (uint8_t)(((reg << 1) & 0x7E) | 0x80), 0x00 };
    uint8_t rx[2] = { 0 };
    spi_transaction_t t = {
        .length = 16,
        .tx_buffer = tx,
        .rx_buffer = rx,
    };
    gpio_set_level(CONFIG_BOARD_RFID_CS_GPIO, 0);
    spi_device_transmit(s_spi, &t);
    gpio_set_level(CONFIG_BOARD_RFID_CS_GPIO, 1);
    return rx[1];
}

static void rfid_set_bitmask(uint8_t reg, uint8_t mask)
{
    rfid_write_reg(reg, rfid_read_reg(reg) | mask);
}

static void rfid_clear_bitmask(uint8_t reg, uint8_t mask)
{
    rfid_write_reg(reg, rfid_read_reg(reg) & (uint8_t)~mask);
}

static esp_err_t rfid_to_card(uint8_t cmd, uint8_t *send_data, uint8_t send_len, uint8_t *back_data, uint8_t *back_len)
{
    uint8_t irq = 0x00;
    uint8_t wait_irq = 0x00;
    uint8_t last_bits;
    uint8_t n;
    uint16_t i;

    switch (cmd) {
    case MFRC522_CMD_TRANSCEIVE:
        wait_irq = 0x30;
        break;
    default:
        return ESP_ERR_INVALID_ARG;
    }

    rfid_write_reg(MFRC522_REG_COMIRQ, (uint8_t)~0x80);
    rfid_set_bitmask(MFRC522_REG_FIFO_LEVEL, 0x80);
    rfid_write_reg(MFRC522_REG_COMMAND, MFRC522_CMD_IDLE);

    for (i = 0; i < send_len; i++) {
        rfid_write_reg(MFRC522_REG_FIFO_DATA, send_data[i]);
    }
    rfid_write_reg(MFRC522_REG_COMMAND, cmd);
    if (cmd == MFRC522_CMD_TRANSCEIVE) {
        rfid_set_bitmask(MFRC522_REG_BIT_FRAMING, 0x80);
    }

    for (i = 2000; i > 0; i--) {
        irq = rfid_read_reg(MFRC522_REG_COMIRQ);
        if (irq & wait_irq) {
            break;
        }
    }

    rfid_clear_bitmask(MFRC522_REG_BIT_FRAMING, 0x80);

    if (i == 0) {
        return ESP_ERR_TIMEOUT;
    }

    if ((rfid_read_reg(MFRC522_REG_ERROR) & 0x1B) != 0x00) {
        return ESP_FAIL;
    }

    if (cmd == MFRC522_CMD_TRANSCEIVE) {
        n = rfid_read_reg(MFRC522_REG_FIFO_LEVEL);
        last_bits = rfid_read_reg(MFRC522_REG_CONTROL) & 0x07;
        if (last_bits) {
            *back_len = (uint8_t)((n - 1) * 8 + last_bits);
        } else {
            *back_len = (uint8_t)(n * 8);
        }
        if (n == 0) {
            n = 1;
        }
        if (n > 16) {
            n = 16;
        }
        for (i = 0; i < n; i++) {
            back_data[i] = rfid_read_reg(MFRC522_REG_FIFO_DATA);
        }
    }

    return ESP_OK;
}

static esp_err_t rfid_request(uint8_t req_mode, uint8_t *tag_type)
{
    esp_err_t status;
    uint8_t back_len = 0;
    uint8_t buffer[18];

    rfid_write_reg(MFRC522_REG_BIT_FRAMING, 0x07);
    buffer[0] = req_mode;
    status = rfid_to_card(MFRC522_CMD_TRANSCEIVE, buffer, 1, buffer, &back_len);
    if (status != ESP_OK || back_len != 0x10) {
        return ESP_FAIL;
    }
    *tag_type = buffer[0];
    return ESP_OK;
}

static esp_err_t rfid_anticoll(uint8_t *serial)
{
    esp_err_t status;
    uint8_t back_len = 0;
    uint8_t buffer[18];
    uint8_t check = 0;
    uint8_t i;

    buffer[0] = MFRC522_PICC_ANTICOLL;
    buffer[1] = 0x20;
    status = rfid_to_card(MFRC522_CMD_TRANSCEIVE, buffer, 2, buffer, &back_len);
    if (status != ESP_OK) {
        return status;
    }
    if (back_len != 40) {
        return ESP_FAIL;
    }

    for (i = 0; i < 4; i++) {
        serial[i] = buffer[i];
        check ^= buffer[i];
    }
    if (check != buffer[4]) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

static void rfid_antenna_on(void)
{
    uint8_t value = rfid_read_reg(MFRC522_REG_TX_CONTROL);
    if ((value & 0x03) != 0x03) {
        rfid_set_bitmask(MFRC522_REG_TX_CONTROL, 0x03);
    }
}

static void rfid_init_chip(void)
{
    rfid_write_reg(MFRC522_REG_COMMAND, MFRC522_CMD_SOFTRESET);
    vTaskDelay(pdMS_TO_TICKS(50));
    rfid_write_reg(MFRC522_REG_MODE, 0x3D);
    rfid_write_reg(MFRC522_REG_TX_AUTO, 0x40);
    rfid_write_reg(MFRC522_REG_TX_CONTROL, 0x00);
    rfid_antenna_on();
}

esp_err_t bsp_rfid_init(void)
{
    gpio_config_t rst_cfg = {
        .pin_bit_mask = 1ULL << CONFIG_BOARD_RFID_RST_GPIO,
        .mode = GPIO_MODE_OUTPUT,
    };
    ESP_ERROR_CHECK(gpio_config(&rst_cfg));

    gpio_config_t cs_cfg = {
        .pin_bit_mask = 1ULL << CONFIG_BOARD_RFID_CS_GPIO,
        .mode = GPIO_MODE_OUTPUT,
    };
    ESP_ERROR_CHECK(gpio_config(&cs_cfg));
    gpio_set_level(CONFIG_BOARD_RFID_CS_GPIO, 1);

    gpio_set_level(CONFIG_BOARD_RFID_RST_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(CONFIG_BOARD_RFID_RST_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(50));

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = CONFIG_BOARD_RFID_MOSI_GPIO,
        .miso_io_num = CONFIG_BOARD_RFID_MISO_GPIO,
        .sclk_io_num = CONFIG_BOARD_RFID_SCK_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 64,
    };

    spi_host_device_t host = (CONFIG_BOARD_RFID_SPI_HOST == 2) ? SPI3_HOST : SPI2_HOST;
    ESP_RETURN_ON_ERROR(spi_bus_initialize(host, &bus_cfg, SPI_DMA_CH_AUTO), TAG, "spi bus");

    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = 1000000,
        .mode = 0,
        .spics_io_num = -1,
        .queue_size = 1,
    };
    ESP_RETURN_ON_ERROR(spi_bus_add_device(host, &dev_cfg, &s_spi), TAG, "spi dev");

    rfid_init_chip();
    uint8_t version = rfid_read_reg(MFRC522_REG_VERSION);
    ESP_LOGI(TAG, "RC522 version=0x%02X", version);
    if (version == 0x00 || version == 0xFF) {
        ESP_LOGW(TAG, "RC522 not detected - RFID KB switching disabled");
    }
    return ESP_OK;
}

bool bsp_rfid_read_uid(char *uid_hex, size_t uid_hex_len)
{
    uint8_t tag_type = 0;
    uint8_t serial[5] = {0};

    if (!uid_hex || uid_hex_len < 9) {
        return false;
    }

    if (rfid_request(MFRC522_PICC_REQIDL, &tag_type) != ESP_OK) {
        return false;
    }
    if (rfid_anticoll(serial) != ESP_OK) {
        return false;
    }

    snprintf(uid_hex, uid_hex_len, "%02X%02X%02X%02X", serial[0], serial[1], serial[2], serial[3]);
    return true;
}

static void rfid_task(void *arg)
{
    char uid[16];
    while (true) {
        if (bsp_rfid_read_uid(uid, sizeof(uid))) {
            if (strcmp(uid, s_last_uid) != 0) {
                snprintf(s_last_uid, sizeof(s_last_uid), "%s", uid);
                ESP_LOGI(TAG, "RFID card: %s", uid);
                if (s_card_cb) {
                    s_card_cb(uid, s_card_cb_arg);
                }
            }
        } else {
            s_last_uid[0] = '\0';
        }
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}

esp_err_t bsp_rfid_start_task(bsp_rfid_card_cb_t cb, void *user_data)
{
    s_card_cb = cb;
    s_card_cb_arg = user_data;
    BaseType_t ok = xTaskCreatePinnedToCore(rfid_task, "rfid", 4096, NULL, 3, NULL, 0);
    return (ok == pdPASS) ? ESP_OK : ESP_FAIL;
}
