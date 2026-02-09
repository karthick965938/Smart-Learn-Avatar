SET(SOURCES screens/ui_ScreenSetup.c
    screens/ui_ScreenWifiReset.c
    screens/ui_ScreenListen.c
    screens/ui_ScreenReset.c
    ui.c
    ui_helpers.c
    images/ui_img_reset_icon_png.c
    images/ui_img_settings_back_png.c
    images/ui_img_sleep_big_z_png.c
    images/ui_img_sleep_small_z_png.c
    images/ui_img_sleep_mode_png.c
    images/ui_img_speaker_speaker_0_png.c
    images/ui_img_speaker_speaker_1_png.c
    images/ui_img_speaker_speaker_2_png.c
    images/ui_img_speaker_speaker_3_png.c
    images/ui_img_speaker_speaker_4_png.c
    images/ui_img_speaker_speaker_5_png.c
    images/ui_img_avatar_avatar_0_png.c
    images/ui_img_avatar_avatar_1_png.c
    images/ui_img_avatar_avatar_2_png.c
    images/ui_img_avatar_avatar_3_png.c
    images/ui_img_avatar_avatar_4_png.c
    images/ui_img_avatar_avatar_5_png.c
    fonts/ui_font_KaiTiCN20.c
    fonts/ui_font_PingFangEN14.c
    fonts/ui_font_PingFangEN16.c
    fonts/ui_font_PingFangEN20.c)

add_library(ui ${SOURCES})
