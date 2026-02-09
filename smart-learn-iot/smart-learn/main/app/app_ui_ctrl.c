/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "esp_log.h"
#include <string.h>
#include <stdlib.h>

#include "app_ui_ctrl.h"
#include "app_wifi.h"
#include "bsp/esp-bsp.h"

#include "ui_helpers.h"
#include "ui.h"

#define LABEL_WIFI_TEXT                 "Connecting to Wi-Fi\n"
#define LABEL_NOT_WIFI_TEXT                 "Not Connected to Wi-Fi\n"
#define LABEL_WIFI_DOT_COUNT_MAX        (10)
#define WIFI_CHECK_TIMER_INTERVAL_S     (1)
#define REPLY_SCROLL_TIMER_INTERVAL_MS  (1000)
#define REPLY_SCROLL_SPEED              (1)
#define SUBTITLE_TIMER_INTERVAL_MS      (50)   // Check every 50ms for smooth typing animation
#define SUBTITLE_TYPING_CHARS_PER_SECOND (20)  // Typing speed (characters per second) - adjust for typing effect
#define SUBTITLE_TYPING_DELAY_MS        (50)   // Delay between each character (ms)

static char *TAG = "ui_ctrl";

static ui_ctrl_panel_t current_panel = UI_CTRL_PANEL_SLEEP;
static lv_timer_t *scroll_timer_handle = NULL;
static lv_timer_t *subtitle_timer_handle = NULL;
static bool reply_audio_start = false;
static bool reply_audio_end = false;
static bool reply_content_get = false;
static uint16_t content_height = 0;

// Subtitle system - typing animation
static char *subtitle_full_text = NULL;
static int subtitle_displayed_chars = 0;
static uint32_t subtitle_start_time = 0;
static uint32_t subtitle_last_char_time = 0;
static bool subtitle_active = false;  // Flag to track if subtitle system is active
static bool subtitle_blocked = false; // SIMPLE FLAG: When true, subtitle timer does NOTHING

static void reply_content_scroll_timer_handler();
static void wifi_check_timer_handler(lv_timer_t *timer);
static void subtitle_timer_handler(lv_timer_t *timer);

void ui_ctrl_init(void)
{
    bsp_display_lock(0);

    ui_init();

    scroll_timer_handle = lv_timer_create(reply_content_scroll_timer_handler, REPLY_SCROLL_TIMER_INTERVAL_MS / REPLY_SCROLL_SPEED, NULL);
    lv_timer_pause(scroll_timer_handle);

    subtitle_timer_handle = lv_timer_create(subtitle_timer_handler, SUBTITLE_TIMER_INTERVAL_MS, NULL);
    lv_timer_pause(subtitle_timer_handle);

    lv_timer_create(wifi_check_timer_handler, WIFI_CHECK_TIMER_INTERVAL_S * 1000, NULL);

    bsp_display_unlock();
}

static void wifi_check_timer_handler(lv_timer_t *timer)
{
    if (WIFI_STATUS_CONNECTED_OK == wifi_connected_already()) {
        lv_obj_clear_flag(ui_PanelSetupSteps, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_PanelSetupWifi, LV_OBJ_FLAG_HIDDEN);
        lv_timer_del(timer);
        if (ui_get_btn_op_group()) {
            lv_group_remove_all_objs(ui_get_btn_op_group());
            lv_group_add_obj(ui_get_btn_op_group(), ui_ButtonSetup);
        }
    } else if (WIFI_STATUS_CONNECTED_FAILED == wifi_connected_already()) {
        lv_label_set_text(ui_LabelSetupWifi, LABEL_NOT_WIFI_TEXT);
    } else {
        if (strlen(lv_label_get_text(ui_LabelSetupWifi)) >= sizeof(LABEL_WIFI_TEXT) + LABEL_WIFI_DOT_COUNT_MAX + 1) {
            lv_label_set_text(ui_LabelSetupWifi, LABEL_WIFI_TEXT);
        } else {
            lv_label_ins_text(ui_LabelSetupWifi, LV_LABEL_POS_LAST, ".");
        }
    }
}

static void show_panel_timer_handler(struct _lv_timer_t *t)
{
    ui_ctrl_panel_t panel = (ui_ctrl_panel_t)t->user_data;
    lv_obj_t *show_panel = NULL;
    lv_obj_t *hide_panel[3] = { NULL };

    switch (panel) {
    case UI_CTRL_PANEL_SLEEP:
        show_panel = ui_PanelSleep;
        hide_panel[0] = ui_PanelListen;
        hide_panel[1] = ui_PanelGet;
        hide_panel[2] = ui_PanelReply;
        // CRITICAL: Completely stop subtitle system when going to sleep
        subtitle_blocked = true;
        subtitle_active = false;
        lv_timer_pause(subtitle_timer_handle);
        if (subtitle_full_text != NULL) {
            free(subtitle_full_text);
            subtitle_full_text = NULL;
        }
        lv_label_set_text(ui_LabelListenSpeak, " ");
        // Reset audio flags and stop animations when going to sleep
        reply_audio_start = false;
        reply_audio_end = false;
        avatar_stop_animations();
        // Start z animations for sleep panel
        ui_sleep_show_animation();
        break;
    case UI_CTRL_PANEL_LISTEN:
        show_panel = ui_PanelListen;
        hide_panel[0] = ui_PanelSleep;
        hide_panel[1] = ui_PanelGet;
        hide_panel[2] = ui_PanelReply;
        // CRITICAL: Completely stop subtitle system FIRST
        subtitle_blocked = true;
        subtitle_active = false;
        lv_timer_pause(subtitle_timer_handle);
        if (subtitle_full_text != NULL) {
            free(subtitle_full_text);
            subtitle_full_text = NULL;
        }
        // Now set the text
        lv_obj_clear_flag(ui_LabelListenSpeak, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(ui_LabelListenSpeak, "Listening ...");
        lv_obj_invalidate(ui_LabelListenSpeak);
        // Reset flags and timer of reply
        reply_content_get = false;
        reply_audio_start = false;
        reply_audio_end = false;
        lv_timer_pause(scroll_timer_handle);
        // Start listening animation (avatar) - no audio playing
        avatar_stop_animations();
        avatar_listening_Animation(ui_ImageListenBody, 0);
        break;
    case UI_CTRL_PANEL_GET:
        show_panel = ui_PanelGet;
        hide_panel[0] = ui_PanelSleep;
        hide_panel[1] = ui_PanelListen;
        hide_panel[2] = ui_PanelReply;
        // CRITICAL: Completely stop subtitle system FIRST
        subtitle_blocked = true;
        subtitle_active = false;
        lv_timer_pause(subtitle_timer_handle);
        if (subtitle_full_text != NULL) {
            free(subtitle_full_text);
            subtitle_full_text = NULL;
        }
        // Now set the text
        lv_obj_clear_flag(ui_LabelListenSpeak, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(ui_LabelListenSpeak, "Thinking ...");
        lv_obj_invalidate(ui_LabelListenSpeak);
        // Start listening animation (avatar) - no audio playing yet
        avatar_stop_animations();
        avatar_listening_Animation(ui_ImageGetBody, 0);
        break;
    case UI_CTRL_PANEL_REPLY:
        show_panel = ui_PanelReply;
        hide_panel[0] = ui_PanelSleep;
        hide_panel[1] = ui_PanelListen;
        hide_panel[2] = ui_PanelGet;
        // Don't hide LabelListenSpeak - it will show subtitles during TTS
        // lv_obj_add_flag(ui_LabelListenSpeak, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ui_LabelListenSpeak, LV_OBJ_FLAG_HIDDEN);  // Make sure it's visible for subtitles
        
        // CRITICAL: Complete animation cleanup before switching to Reply panel
        avatar_stop_animations();
        
        // Multiple refresh cycles to ensure clean state
        lv_refr_now(NULL);
        lv_timer_handler();
        lv_refr_now(NULL);
        
        // Small delay for cleanup
        vTaskDelay(pdMS_TO_TICKS(10));
        
        // Start appropriate animation based on audio state
        if (reply_audio_start) {
            // Audio is already playing - use speaking animation ONLY for Reply panel
            avatar_speaking_Animation(ui_ImageRelyBody, 0);
        } else {
            // No audio yet - use listening animation, will switch when audio starts
            avatar_listening_Animation(ui_ImageRelyBody, 0);
        }
        break;
    default:
        break;
    }

    lv_obj_clear_flag(show_panel, LV_OBJ_FLAG_HIDDEN);
    for (int i = 0; i < sizeof(hide_panel) / sizeof(lv_obj_t *); i++) {
        lv_obj_add_flag(hide_panel[i], LV_OBJ_FLAG_HIDDEN);
    }

    current_panel = panel;

    ESP_LOGI(TAG, "Swich to panel[%d]", panel);
}

void ui_ctrl_show_panel(ui_ctrl_panel_t panel, uint16_t timeout)
{
    bsp_display_lock(0);

    if (timeout) {
        lv_timer_t *timer = lv_timer_create(show_panel_timer_handler, timeout, NULL);
        timer->user_data = (void *)panel;
        lv_timer_set_repeat_count(timer, 1);
        ESP_LOGW(TAG, "Switch panel to [%d] in %dms", panel, timeout);
    } else {
        lv_timer_t timer;
        timer.user_data = (void *)panel;
        show_panel_timer_handler(&timer);
    }

    bsp_display_unlock();
}

static void reply_content_show_text(const char *text)
{
    if (NULL == text) {
        return;
    }

    char *decode = heap_caps_malloc((strlen(text) + 1), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    assert(decode);

    int j = 0;
    for (int i = 0; i < strlen(text);) {
        if ((*(text + i) == '\\') && ((i + 1) < strlen(text)) && (*(text + i + 1) == 'n')) {
            *(decode + j++) = '\n';
            i += 2;
        } else {
            *(decode + j++) = *(text + i);
            i += 1;
        }
    }
    *(decode + j) = '\0';

    ESP_LOGI(TAG, "decode:[%d, %d] %s\r\n", j, strlen(decode), decode);

    lv_label_set_text(ui_LabelReplyContent, decode);
    content_height = lv_obj_get_self_height(ui_LabelReplyContent);
    lv_obj_scroll_to_y(ui_ContainerReplyContent, 0, LV_ANIM_OFF);
    reply_content_get = true;
    lv_timer_resume(scroll_timer_handle);
    ESP_LOGI(TAG, "reply scroll timer start");

    if (decode) {
        free(decode);
    }
}

void ui_ctrl_label_show_text(ui_ctrl_label_t label, const char *text)
{
    bsp_display_lock(0);

    if (text != NULL) {
        switch (label) {
        case UI_CTRL_LABEL_LISTEN_SPEAK:
            ESP_LOGI(TAG, "update listen speak: %s", text);
            // CRITICAL: Completely stop subtitle system FIRST
            subtitle_blocked = true;
            subtitle_active = false;
            lv_timer_pause(subtitle_timer_handle);
            if (subtitle_full_text != NULL) {
                free(subtitle_full_text);
                subtitle_full_text = NULL;
            }
            // Now set the text (STT / status)
            lv_obj_clear_flag(ui_LabelListenSpeak, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(ui_LabelListenSpeak, text);
            lv_obj_invalidate(ui_LabelListenSpeak);
            break;
        case UI_CTRL_LABEL_REPLY_QUESTION:
            ESP_LOGI(TAG, "update reply question");
            lv_label_set_text(ui_LabelReplyQuestion, text);
            break;
        case UI_CTRL_LABEL_REPLY_CONTENT:
            ESP_LOGI(TAG, "update reply content");
            reply_content_show_text(text);
            break;
        default:
            break;
        }
    }

    bsp_display_unlock();
}

static void anim_callback_set_bg_img_opacity(lv_anim_t *a, int32_t v)
{
    ui_anim_user_data_t *usr = (ui_anim_user_data_t *)a->user_data;
    lv_obj_set_style_bg_img_opa(usr->target, v, 0);
}

static int32_t anim_callback_get_opacity(lv_anim_t *a)
{
    ui_anim_user_data_t *usr = (ui_anim_user_data_t *)a->user_data;
    return lv_obj_get_style_bg_img_opa(usr->target, 0);
}

void ui_sleep_show_animation(void)
{
    bsp_display_lock(0);

    if (!ui_ContainerBigZ || !ui_ContainerSmallZ || !ui_ImageSleepBody) {
        bsp_display_unlock();
        return;
    }

    // Stop existing animations on z containers and body first to prevent conflicts
    lv_anim_del(ui_ContainerBigZ, NULL);
    lv_anim_del(ui_ContainerSmallZ, NULL);
    lv_anim_del(ui_ImageSleepBody, NULL);
    
    // Process any pending animations
    lv_timer_handler();
    
    // Reset to initial state for smooth start
    lv_obj_set_style_bg_img_opa(ui_ContainerBigZ, 0, 0);
    lv_obj_set_style_bg_img_opa(ui_ContainerSmallZ, 0, 0);
    lv_obj_set_y(ui_ImageSleepBody, 0);
    
    // Force immediate refresh
    lv_obj_invalidate(ui_ContainerBigZ);
    lv_obj_invalidate(ui_ContainerSmallZ);
    lv_obj_invalidate(ui_ImageSleepBody);
    
    // Small delay to ensure clean state
    lv_timer_handler();

    // Big Z animation
    ui_anim_user_data_t *PropertyAnimation_0_user_data = lv_mem_alloc(sizeof(ui_anim_user_data_t));
    PropertyAnimation_0_user_data->target = ui_ContainerBigZ;
    PropertyAnimation_0_user_data->val = -1;
    lv_anim_t PropertyAnimation_0;
    lv_anim_init(&PropertyAnimation_0);
    lv_anim_set_var(&PropertyAnimation_0, ui_ContainerBigZ);
    lv_anim_set_time(&PropertyAnimation_0, 1000);
    lv_anim_set_user_data(&PropertyAnimation_0, PropertyAnimation_0_user_data);
    lv_anim_set_custom_exec_cb(&PropertyAnimation_0, anim_callback_set_bg_img_opacity);
    lv_anim_set_values(&PropertyAnimation_0, 0, 255);
    lv_anim_set_path_cb(&PropertyAnimation_0, lv_anim_path_linear);
    lv_anim_set_delay(&PropertyAnimation_0, 0);
    lv_anim_set_deleted_cb(&PropertyAnimation_0, _ui_anim_callback_free_user_data);
    lv_anim_set_playback_time(&PropertyAnimation_0, 1000);
    lv_anim_set_playback_delay(&PropertyAnimation_0, 0);
    lv_anim_set_repeat_count(&PropertyAnimation_0, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_repeat_delay(&PropertyAnimation_0, 1000);
    lv_anim_set_early_apply(&PropertyAnimation_0, false);
    lv_anim_set_get_value_cb(&PropertyAnimation_0, &anim_callback_get_opacity);
    lv_anim_start(&PropertyAnimation_0);

    // Small Z animation
    ui_anim_user_data_t *PropertyAnimation_1_user_data = lv_mem_alloc(sizeof(ui_anim_user_data_t));
    PropertyAnimation_1_user_data->target = ui_ContainerSmallZ;
    PropertyAnimation_1_user_data->val = -1;
    lv_anim_t PropertyAnimation_1;
    lv_anim_init(&PropertyAnimation_1);
    lv_anim_set_var(&PropertyAnimation_1, ui_ContainerSmallZ);
    lv_anim_set_time(&PropertyAnimation_1, 1000);
    lv_anim_set_user_data(&PropertyAnimation_1, PropertyAnimation_1_user_data);
    lv_anim_set_custom_exec_cb(&PropertyAnimation_1, anim_callback_set_bg_img_opacity);
    lv_anim_set_values(&PropertyAnimation_1, 0, 255);
    lv_anim_set_path_cb(&PropertyAnimation_1, lv_anim_path_linear);
    lv_anim_set_delay(&PropertyAnimation_1, 1000);
    lv_anim_set_deleted_cb(&PropertyAnimation_1, _ui_anim_callback_free_user_data);
    lv_anim_set_playback_time(&PropertyAnimation_1, 1000);
    lv_anim_set_playback_delay(&PropertyAnimation_1, 0);
    lv_anim_set_repeat_count(&PropertyAnimation_1, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_repeat_delay(&PropertyAnimation_1, 1000);
    lv_anim_set_early_apply(&PropertyAnimation_1, false);
    lv_anim_set_get_value_cb(&PropertyAnimation_1, &anim_callback_get_opacity);
    lv_anim_start(&PropertyAnimation_1);

    // Body Floating Animation (from provided configuration)
    // Start: 0, End: 5, Relative, Time: 2000, Repeat: Infinite
    ui_anim_user_data_t *PropertyAnimation_2_user_data = lv_mem_alloc(sizeof(ui_anim_user_data_t));
    PropertyAnimation_2_user_data->target = ui_ImageSleepBody;
    PropertyAnimation_2_user_data->val = -1;
    lv_anim_t PropertyAnimation_2;
    lv_anim_init(&PropertyAnimation_2);
    lv_anim_set_var(&PropertyAnimation_2, ui_ImageSleepBody);
    lv_anim_set_time(&PropertyAnimation_2, 2000);
    lv_anim_set_user_data(&PropertyAnimation_2, PropertyAnimation_2_user_data);
    lv_anim_set_custom_exec_cb(&PropertyAnimation_2, _ui_anim_callback_set_y);
    lv_anim_set_values(&PropertyAnimation_2, 0, 5);
    lv_anim_set_path_cb(&PropertyAnimation_2, lv_anim_path_linear);
    lv_anim_set_delay(&PropertyAnimation_2, 0);
    lv_anim_set_deleted_cb(&PropertyAnimation_2, _ui_anim_callback_free_user_data);
    lv_anim_set_playback_time(&PropertyAnimation_2, 0);
    lv_anim_set_playback_delay(&PropertyAnimation_2, 0);
    lv_anim_set_repeat_count(&PropertyAnimation_2, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_repeat_delay(&PropertyAnimation_2, 0);
    lv_anim_set_early_apply(&PropertyAnimation_2, false);
    lv_anim_set_get_value_cb(&PropertyAnimation_2, &_ui_anim_callback_get_y);
    lv_anim_start(&PropertyAnimation_2);

    bsp_display_unlock();
}

void ui_ctrl_reply_set_audio_start_flag(bool result)
{
    bsp_display_lock(0);
    
    reply_audio_start = result;
    if (result) {
        // Audio/TTS started playing.
        // STEP 1: hide STT/status text while TTS is playing – keep label empty.
        lv_obj_clear_flag(ui_LabelListenSpeak, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(ui_LabelListenSpeak, "");
        lv_obj_invalidate(ui_LabelListenSpeak);
        
        // CRITICAL: Stop ALL animations globally first
        avatar_stop_animations();
        
        // Force multiple refresh cycles to completely clear animation queue
        lv_refr_now(NULL);
        lv_timer_handler();
        lv_refr_now(NULL);
        
        // Wait a moment for cleanup
        vTaskDelay(pdMS_TO_TICKS(10));
        
        // Now start ONLY speaker animation for Reply panel (if visible)
        if (!lv_obj_has_flag(ui_PanelReply, LV_OBJ_FLAG_HIDDEN)) {
            // Only Reply panel gets speaker animation during TTS
            avatar_speaking_Animation(ui_ImageRelyBody, 0);
        }
        // Listen and Get panels stay with avatar animation (but we're on Reply panel during TTS)
    }
    
    bsp_display_unlock();
}

void ui_ctrl_subtitle_start(const char *text)
{
    if (text == NULL || strlen(text) == 0) {
        return;
    }

    bsp_display_lock(0);
    
    // Store the response text so we can show it when audio starts
    if (subtitle_full_text != NULL) {
        free(subtitle_full_text);
        subtitle_full_text = NULL;
    }
    subtitle_full_text = (char *)malloc(strlen(text) + 1);
    if (subtitle_full_text != NULL) {
        strcpy(subtitle_full_text, text);
    }

    // No timer, no typing animation in STEP 1
    bsp_display_unlock();
}

void ui_ctrl_subtitle_stop(void)
{
    bsp_display_lock(0);
    
    // SIMPLE APPROACH: Block subtitle timer and cleanup
    subtitle_blocked = true;
    subtitle_active = false;
    
    if (subtitle_full_text != NULL) {
        free(subtitle_full_text);
        subtitle_full_text = NULL;
    }
    
    subtitle_displayed_chars = 0;
    subtitle_start_time = 0;
    subtitle_last_char_time = 0;
    
    lv_timer_pause(subtitle_timer_handle);
    
    bsp_display_unlock();
}

bool ui_ctrl_reply_get_audio_start_flag(void)
{
    return reply_audio_start;
}

void ui_ctrl_reply_set_audio_end_flag(bool result)
{
    bsp_display_lock(0);
    
    reply_audio_end = result;
    if (result) {
        // CRITICAL: Completely stop subtitle system when audio ends
        subtitle_blocked = true;
        subtitle_active = false;
        lv_timer_pause(subtitle_timer_handle);
        if (subtitle_full_text != NULL) {
            free(subtitle_full_text);
            subtitle_full_text = NULL;
        }
        subtitle_displayed_chars = 0;
        subtitle_start_time = 0;
        subtitle_last_char_time = 0;
        
        // CRITICAL: Stop ALL animations globally first
        avatar_stop_animations();
        
        // Force multiple refresh cycles to completely clear animation queue
        lv_refr_now(NULL);
        lv_timer_handler();
        lv_refr_now(NULL);
        
        // Wait a moment for cleanup
        vTaskDelay(pdMS_TO_TICKS(10));
        
        // Now start avatar animation for the currently visible panel only
        if (!lv_obj_has_flag(ui_PanelReply, LV_OBJ_FLAG_HIDDEN)) {
            avatar_listening_Animation(ui_ImageRelyBody, 0);
        } else if (!lv_obj_has_flag(ui_PanelListen, LV_OBJ_FLAG_HIDDEN)) {
            avatar_listening_Animation(ui_ImageListenBody, 0);
        } else if (!lv_obj_has_flag(ui_PanelGet, LV_OBJ_FLAG_HIDDEN)) {
            avatar_listening_Animation(ui_ImageGetBody, 0);
        }
    }
    
    bsp_display_unlock();
}

static void subtitle_timer_handler(lv_timer_t *timer)
{
    LV_UNUSED(timer);
    // STEP 1: Subtitles are temporarily disabled to stabilize UI.
    // Do nothing here so subtitle logic can NOT override
    // "Listening...", "Thinking...", or STT text.
    return;
}

static void reply_content_scroll_timer_handler()
{
    lv_coord_t offset = 0;
    const lv_font_t *font = NULL;

    if (reply_content_get && reply_audio_start) {
        font = lv_obj_get_style_text_font(ui_LabelReplyContent, 0);
        offset = lv_obj_get_scroll_y(ui_ContainerReplyContent);
        // ESP_LOGI(TAG, "offset: %d, content_height: %d, font_height: %d", offset, content_height, font->line_height);
        if ((content_height > lv_obj_get_height(ui_ContainerReplyContent)) &&
                (offset < (content_height - lv_obj_get_height(ui_ContainerReplyContent)))) {
            offset += font->line_height / 2;
            lv_obj_scroll_to_y(ui_ContainerReplyContent, offset, LV_ANIM_OFF);
        } else if (reply_audio_end) {
            ESP_LOGI(TAG, "reply scroll timer stop");
            reply_content_get = false;
            reply_audio_start = false;
            reply_audio_end = false;
            lv_timer_pause(scroll_timer_handle);
            // Switch back to listening animation before going to sleep
            avatar_stop_animations();
            ui_ctrl_show_panel(UI_CTRL_PANEL_SLEEP, 300);
        }
    }
}

void ui_ctrl_guide_jump( void )
{
    lv_obj_t *act_scr = lv_scr_act();
    if (act_scr == ui_ScreenSetup) {
        ESP_LOGI(TAG, "act_scr:%p, ui_ScreenSetup:%p", act_scr, ui_ScreenSetup);
        lv_event_send(ui_ButtonSetup, LV_EVENT_CLICKED, 0);
    }
}
