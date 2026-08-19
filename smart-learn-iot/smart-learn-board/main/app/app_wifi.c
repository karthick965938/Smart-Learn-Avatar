#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_check.h"
#include "esp_log.h"
#include "app_wifi.h"
#include "settings.h"

#define EXAMPLE_ESP_MAXIMUM_RETRY CONFIG_ESP_MAXIMUM_RETRY

#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
#define portTICK_RATE_MS 10

static const char *TAG = "wifi station";
static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;
static bool s_reconnect = true;
static bool wifi_connected = false;
static QueueHandle_t wifi_event_queue = NULL;

scan_info_t scan_info_result = {
    .scan_done = WIFI_SCAN_IDLE,
    .ap_count = 0,
};

WiFi_Connect_Status wifi_connected_already(void)
{
    if (wifi_connected) {
        return WIFI_STATUS_CONNECTED_OK;
    }
    return (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) ? WIFI_STATUS_CONNECTING : WIFI_STATUS_CONNECTED_FAILED;
}

esp_err_t send_network_event(net_event_t event)
{
    net_event_t eventOut = event;
    if (NET_EVENT_RECONNECT == event) {
        wifi_connected = false;
    }
    ESP_RETURN_ON_FALSE(xQueueSend(wifi_event_queue, &eventOut, 0) == pdPASS,
                        ESP_ERR_INVALID_STATE, TAG, "event queue busy");
    return ESP_OK;
}

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        send_network_event(NET_EVENT_POWERON_SCAN);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_reconnect && ++s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
        }
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        wifi_connected = false;
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        wifi_connected = true;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, NULL));

    wifi_config_t wifi_config = {0};
    sys_param_t *sys_param = settings_get_parameter();
    memcpy(wifi_config.sta.ssid, sys_param->ssid, sizeof(wifi_config.sta.ssid));
    memcpy(wifi_config.sta.password, sys_param->password, sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "WiFi starting: %s", wifi_config.sta.ssid);
}

static void network_task(void *args)
{
    (void)args;
    wifi_init_sta();

    net_event_t net_event;
    while (1) {
        if (xQueueReceive(wifi_event_queue, &net_event, portTICK_RATE_MS / 5) == pdPASS) {
            if (net_event == NET_EVENT_POWERON_SCAN) {
                esp_wifi_connect();
                wifi_connected = false;
            }
        }
    }
}

bool app_wifi_lock(uint32_t timeout_ms)
{
    const TickType_t timeout_ticks = (timeout_ms == 0) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTakeRecursive(scan_info_result.wifi_mux, timeout_ticks) == pdTRUE;
}

void app_wifi_unlock(void)
{
    xSemaphoreGiveRecursive(scan_info_result.wifi_mux);
}

void app_wifi_state_set(wifi_scan_status_t status)
{
    app_wifi_lock(0);
    scan_info_result.scan_done = status;
    app_wifi_unlock();
}

void app_network_start(void)
{
    scan_info_result.wifi_mux = xSemaphoreCreateRecursiveMutex();
    wifi_event_queue = xQueueCreate(4, sizeof(net_event_t));
    xTaskCreatePinnedToCore(network_task, "NetWork Task", 5 * 1024, NULL, 1, NULL, 0);
}
