#pragma once

#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BSP_OLED_WIDTH  128
#define BSP_OLED_HEIGHT 64

esp_err_t bsp_oled_init(void);
void bsp_oled_clear(void);
void bsp_oled_display(void);
void bsp_oled_set_line(uint8_t line, const char *text);
void bsp_oled_set_title(const char *text);
void bsp_oled_set_body(const char *text);

#ifdef __cplusplus
}
#endif
