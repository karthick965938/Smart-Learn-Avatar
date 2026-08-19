idf_component_register(
    SRCS
        "src/bsp_board.c"
        "src/bsp_i2c.c"
        "src/bsp_i2s.c"
        "src/bsp_oled.c"
        "src/bsp_rfid.c"
        "src/bsp_spiffs.c"
    INCLUDE_DIRS "include"
    REQUIRES driver esp_lcd nvs_flash spiffs fatfs
)
