#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "app_ui_ctrl.h"
#include "app_wifi.h"
#include "bsp_oled.h"
#include "settings.h"

#define UI_OLED_LINE_LEN 22
#define WIFI_CHECK_INTERVAL_MS 1000
#define WIFI_DOT_COUNT_MAX 3

static ui_ctrl_panel_t s_panel = UI_CTRL_PANEL_SLEEP;
static char s_status_line[UI_OLED_LINE_LEN] = "Starting...";
static char s_question[128];
static char s_answer[256];
static char s_kb_label[UI_OLED_LINE_LEN] = "Default";
static bool s_reply_audio_start;
static bool s_reply_audio_end;
static bool s_wifi_ready;
static TimerHandle_t s_wifi_timer;

static void render(void)
{
    bsp_oled_set_title(s_kb_label);
    bsp_oled_set_line(0, s_status_line);

    if (s_panel == UI_CTRL_PANEL_REPLY) {
        bsp_oled_set_line(1, "Q:");
        bsp_oled_set_body(s_answer[0] ? s_answer : s_question);
    } else if (s_question[0]) {
        bsp_oled_set_line(1, "You:");
        bsp_oled_set_body(s_question);
    } else {
        bsp_oled_set_line(1, "");
        bsp_oled_set_line(2, "");
    }

    bsp_oled_display();
}

static void wifi_ready_timer_cb(TimerHandle_t timer)
{
    (void)timer;
    ui_ctrl_show_panel(UI_CTRL_PANEL_SLEEP, 0);
}

static void wifi_check_timer_cb(TimerHandle_t timer)
{
    (void)timer;

    if (s_panel != UI_CTRL_PANEL_SLEEP || s_wifi_ready) {
        return;
    }

    WiFi_Connect_Status status = wifi_connected_already();

    if (status == WIFI_STATUS_CONNECTED_OK) {
        s_wifi_ready = true;
        strcpy(s_status_line, "WiFi connected");
        render();

        if (s_wifi_timer) {
            xTimerStop(s_wifi_timer, 0);
            xTimerDelete(s_wifi_timer, 0);
            s_wifi_timer = NULL;
        }

        TimerHandle_t ready_timer = xTimerCreate("wifi_ok", pdMS_TO_TICKS(1500), pdFALSE, NULL, wifi_ready_timer_cb);
        if (ready_timer) {
            xTimerStart(ready_timer, 0);
        }
        return;
    }

    if (status == WIFI_STATUS_CONNECTED_FAILED) {
        strcpy(s_status_line, "WiFi failed");
        render();
        if (s_wifi_timer) {
            xTimerStop(s_wifi_timer, 0);
            xTimerDelete(s_wifi_timer, 0);
            s_wifi_timer = NULL;
        }
        return;
    }

    static uint8_t dot_count;
    dot_count = (dot_count + 1) % (WIFI_DOT_COUNT_MAX + 1);
    snprintf(s_status_line, sizeof(s_status_line), "Connecting WiFi%.*s", dot_count, "...");
    render();
}

static void panel_timer_cb(TimerHandle_t timer)
{
    ui_ctrl_panel_t panel = (ui_ctrl_panel_t)(uintptr_t)pvTimerGetTimerID(timer);
    ui_ctrl_show_panel(panel, 0);
    xTimerDelete(timer, 0);
}

void ui_ctrl_init(void)
{
    s_wifi_ready = false;
    strcpy(s_status_line, "Connecting WiFi");
    snprintf(s_kb_label, sizeof(s_kb_label), "%.*s",
             (int)(sizeof(s_kb_label) - 1), settings_get_active_kb_label());
    render();

    s_wifi_timer = xTimerCreate("wifi_chk", pdMS_TO_TICKS(WIFI_CHECK_INTERVAL_MS),
                                pdTRUE, NULL, wifi_check_timer_cb);
    if (s_wifi_timer) {
        xTimerStart(s_wifi_timer, 0);
    }
}

void ui_ctrl_set_kb_label(const char *label)
{
    if (!label) {
        return;
    }
    snprintf(s_kb_label, sizeof(s_kb_label), "%.*s",
             (int)(sizeof(s_kb_label) - 1), label);
    render();
}

void ui_ctrl_show_panel(ui_ctrl_panel_t panel, uint16_t timeout)
{
    s_panel = panel;
    switch (panel) {
    case UI_CTRL_PANEL_SLEEP:
        strcpy(s_status_line, "Say 'Hi Json' to ask");
        break;
    case UI_CTRL_PANEL_LISTEN:
        strcpy(s_status_line, "Listening...");
        break;
    case UI_CTRL_PANEL_GET:
        strcpy(s_status_line, "Thinking...");
        break;
    case UI_CTRL_PANEL_REPLY:
        strcpy(s_status_line, s_reply_audio_start ? "Speaking..." : "Answer ready");
        break;
    default:
        break;
    }
    render();

    if (timeout > 0) {
        TimerHandle_t timer = xTimerCreate("ui_panel", pdMS_TO_TICKS(timeout), pdFALSE,
                                           (void *)(uintptr_t)UI_CTRL_PANEL_SLEEP, panel_timer_cb);
        if (timer) {
            xTimerStart(timer, 0);
        }
    }
}

void ui_ctrl_label_show_text(ui_ctrl_label_t label, const char *text)
{
    if (!text) {
        text = "";
    }

    switch (label) {
    case UI_CTRL_LABEL_LISTEN_SPEAK:
        snprintf(s_question, sizeof(s_question), "%s", text);
        break;
    case UI_CTRL_LABEL_REPLY_QUESTION:
        snprintf(s_question, sizeof(s_question), "%s", text);
        break;
    case UI_CTRL_LABEL_REPLY_CONTENT:
        snprintf(s_answer, sizeof(s_answer), "%s", text);
        break;
    default:
        break;
    }
    render();
}

void ui_ctrl_guide_jump(void)
{
    s_question[0] = '\0';
    s_answer[0] = '\0';
}

void ui_ctrl_reply_set_audio_start_flag(bool result)
{
    s_reply_audio_start = result;
    if (result) {
        strcpy(s_status_line, "Speaking...");
        render();
    }
}

bool ui_ctrl_reply_get_audio_start_flag(void)
{
    return s_reply_audio_start;
}

void ui_ctrl_reply_set_audio_end_flag(bool result)
{
    s_reply_audio_end = result;
    if (result) {
        s_reply_audio_start = false;
        ui_ctrl_show_panel(UI_CTRL_PANEL_SLEEP, 2000);
    }
}

void ui_ctrl_subtitle_start(const char *text)
{
    ui_ctrl_label_show_text(UI_CTRL_LABEL_REPLY_CONTENT, text);
}

void ui_ctrl_subtitle_stop(void)
{
}
