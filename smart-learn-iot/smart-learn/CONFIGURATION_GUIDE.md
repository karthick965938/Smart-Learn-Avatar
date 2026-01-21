# Smart Learn Avatar - Configuration Guide

## Overview
This guide explains how to configure the TTS voice, UI theme, and colors for your Smart Learn Avatar application.

## Configuration Options

### 1. TTS Voice Configuration

The application supports multiple OpenAI TTS voices that can be configured via NVS (Non-Volatile Storage).

#### Available Voices

**Male Voices:**
- `alloy` - Neutral male voice
- `echo` - Deep male voice
- `onyx` - Rich male voice

**Female Voices:**
- `fable` - Expressive female voice
- `nova` - Warm female voice
- `shimmer` - Clear female voice (default)

#### Configuration Methods

**Method 1: Via menuconfig (Build-time)**
```bash
idf.py menuconfig
# Navigate to: Example Configuration -> TTS Voice Selection
# Select your preferred voice
# Save and exit
idf.py build flash
```

**Method 2: Via factory_nvs (Factory defaults)**
```bash
cd factory_nvs
idf.py menuconfig
# Navigate to: Example Configuration -> TTS Voice Selection
# Select your preferred voice
idf.py build
cd ..
idf.py build flash
```

**Method 3: Via NVS at runtime**
You can modify the NVS value programmatically or via UF2 configuration:
- NVS Key: `tts_voice`
- Type: String
- Valid values: `"alloy"`, `"echo"`, `"onyx"`, `"fable"`, `"nova"`, `"shimmer"`

---

### 2. UI Theme Configuration

The application supports three theme types with customizable colors.

#### Theme Types

1. **Dark Theme** (default)
   - Background: Black (0x000000)
   - Text: White (0xFFFFFF)
   - Button: Green (0x04B900)

2. **Light Theme**
   - Background: White (0xFFFFFF)
   - Text: Black (0x000000)
   - Button: Green (0x04B900)

3. **Custom Theme**
   - Fully customizable colors
   - Default Custom: Dark blue background (0x1a1a2e), White text, Green button

#### Configuration Methods

**Method 1: Via menuconfig (Build-time)**
```bash
idf.py menuconfig
# Navigate to: Example Configuration -> UI Theme
# Select theme type (Dark/Light/Custom)
# If Custom selected, configure:
#   - Background Color (RGB)
#   - Text Color (RGB)
#   - Button/Primary Color (RGB)
idf.py build flash
```

**Method 2: Via factory_nvs (Factory defaults)**
```bash
cd factory_nvs
idf.py menuconfig
# Navigate to: Example Configuration -> UI Theme
# Configure theme and colors
idf.py build
cd ..
idf.py build flash
```

**Method 3: Via NVS at runtime**
You can modify these NVS values:
- `theme_type` (u8): 0=Dark, 1=Light, 2=Custom
- `bg_color` (u32): Background color in RGB hex (e.g., 0x000000)
- `text_color` (u32): Text color in RGB hex (e.g., 0xFFFFFF)
- `button_color` (u32): Button/primary color in RGB hex (e.g., 0x04B900)

---

## Color Format

All colors are specified in RGB hex format: `0xRRGGBB`

### Common Color Examples

| Color | Hex Value | Description |
|-------|-----------|-------------|
| Black | 0x000000 | Pure black |
| White | 0xFFFFFF | Pure white |
| Red | 0xFF0000 | Pure red |
| Green | 0x00FF00 | Pure green |
| Blue | 0x0000FF | Pure blue |
| Custom Green | 0x04B900 | App default green |
| Orange | 0xD24B09 | App secondary color |
| Dark Blue | 0x1a1a2e | Custom theme default |

---

## NVS Storage Structure

All configurable parameters are stored in NVS:

```
Partition: "nvs"
Namespace: "configuration"

Keys:
├── ssid (string)          - WiFi SSID
├── password (string)      - WiFi Password
├── ChatGPT_key (string)   - OpenAI API Key
├── Base_url (string)      - OpenAI Base URL
├── KB_url (string)        - Knowledge Base URL
├── tts_voice (string)     - TTS Voice name
├── theme_type (u8)        - Theme type (0/1/2)
├── bg_color (u32)         - Background color
├── text_color (u32)       - Text color
└── button_color (u32)     - Button color
```

---

## Examples

### Example 1: Configure Male Voice with Dark Theme
```bash
idf.py menuconfig
# TTS Voice Selection -> echo (male)
# UI Theme -> Dark Theme
idf.py build flash
```

### Example 2: Custom Purple Theme
```bash
idf.py menuconfig
# UI Theme -> Custom Theme
# Background Color (RGB): 0x2d1b69  (Dark purple)
# Text Color (RGB): 0xFFFFFF          (White)
# Button/Primary Color (RGB): 0x9b59b6 (Light purple)
idf.py build flash
```

### Example 3: Light Theme with Female Voice
```bash
idf.py menuconfig
# TTS Voice Selection -> nova (female)
# UI Theme -> Light Theme
idf.py build flash
```

---

## Programmatic Access

### In Your Code

```c
#include "settings.h"
#include "app_theme.h"

void your_function(void) {
    // Get system parameters
    sys_param_t *sys_param = settings_get_parameter();
    
    // Access voice setting
    ESP_LOGI(TAG, "Current TTS Voice: %s", sys_param->tts_voice);
    
    // Access theme settings
    ESP_LOGI(TAG, "Theme Type: %d", sys_param->theme_type);
    ESP_LOGI(TAG, "BG Color: 0x%06X", sys_param->bg_color);
    ESP_LOGI(TAG, "Text Color: 0x%06X", sys_param->text_color);
    ESP_LOGI(TAG, "Button Color: 0x%06X", sys_param->button_color);
    
    // Apply theme
    app_theme_apply(sys_param);
    
    // Get individual colors for custom use
    lv_color_t bg = app_theme_get_bg_color(sys_param);
    lv_color_t text = app_theme_get_text_color(sys_param);
    lv_color_t button = app_theme_get_button_color(sys_param);
}
```

---

## Troubleshooting

### Voice not changing
1. Ensure the voice name is spelled correctly in NVS
2. Check that OpenAI TTS API supports the voice
3. Verify NVS is properly initialized
4. Check logs for TTS voice loading confirmation

### Theme not applying
1. Ensure theme is applied after UI initialization
2. Check NVS values are within valid ranges
3. Verify color values are in correct format (0xRRGGBB)
4. Check logs for theme application messages

### Factory reset to defaults
To reset all settings to factory defaults:
```bash
idf.py erase-flash
cd factory_nvs
idf.py build flash
cd ..
idf.py build flash monitor
```

---

## Technical Details

### Theme Application
- Themes are applied during `app_main()` initialization
- Theme settings are loaded from NVS during boot
- LVGL theme system is used for consistent styling
- Colors are converted from RGB hex to LVGL color format

### Voice Selection
- Voice is set during OpenAI TTS initialization
- Voice string is passed directly to OpenAI API
- Invalid voices will fall back to default ("shimmer")

---

## Future Enhancements

Potential future additions:
- [ ] Runtime theme switching without reboot
- [ ] Voice preview/testing interface
- [ ] Additional theme presets
- [ ] Per-screen color customization
- [ ] Gradient background support
- [ ] Multiple voice profiles

---

## Support

For issues or questions:
1. Check ESP-IDF logs for error messages
2. Verify NVS partition is not corrupted
3. Ensure factory_nvs is built before main app
4. Review this guide for configuration steps

---

**Last Updated:** 2026-01-21
**Version:** 1.0
