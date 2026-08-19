#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*bsp_rfid_card_cb_t)(const char *uid_hex, void *user_data);

esp_err_t bsp_rfid_init(void);
esp_err_t bsp_rfid_start_task(bsp_rfid_card_cb_t cb, void *user_data);
bool bsp_rfid_read_uid(char *uid_hex, size_t uid_hex_len);

#ifdef __cplusplus
}
#endif
