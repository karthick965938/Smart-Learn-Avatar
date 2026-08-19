set(APP_DIR ./app)
file(GLOB_RECURSE APP_SRCS ${APP_DIR}/*.c)

set(SETTINGS_DIR ./settings)
file(GLOB_RECURSE SETTINGS_SRCS ${SETTINGS_DIR}/*.c)

idf_component_register(
    SRCS
        "main.c"
        ${APP_SRCS}
        ${SETTINGS_SRCS}
    INCLUDE_DIRS
        "."
        ${APP_DIR}
        ${SETTINGS_DIR}
    REQUIRES
        bsp-custom
        nvs_flash
        esp_wifi
        esp_event
        app_update
        esp_http_client
        esp-tls
        json
        mbedtls
)

set(nvs_src_file ${PROJECT_DIR}/../smart-learn/factory_nvs/build/factory_nvs.bin)
set(nvs_dst_file ${CMAKE_BINARY_DIR}/uf2/factory_nvs.bin)

if(NOT EXISTS ${nvs_src_file})
    message(WARNING "factory_nvs.bin not found. Build ../smart-learn/factory_nvs first.")
else()
    add_custom_target(nvs_uf2_bin ALL)
    add_custom_command(
        TARGET nvs_uf2_bin
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/uf2
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${nvs_src_file} ${nvs_dst_file}
        VERBATIM)
    add_dependencies(flash nvs_uf2_bin)

    partition_table_get_partition_info(size "--partition-name ota_0" "size")
    partition_table_get_partition_info(offset "--partition-name ota_0" "offset")
    if("${size}" AND "${offset}")
        esptool_py_flash_to_partition(flash "ota_0" "${nvs_dst_file}")
    endif()
endif()

spiffs_create_partition_image(storage ../../smart-learn/spiffs FLASH_IN_PROJECT)
