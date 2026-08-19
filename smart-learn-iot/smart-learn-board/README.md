# Smart Learn Board Firmware

Firmware for a custom **ESP32-S3** board with:

- **INMP441** I2S microphone
- **MAX98357A** I2S amplifier/speaker
- **RC522** RFID reader (API-driven knowledge-base switching)
- **SH1106** 128x64 OLED (simple text UI, no avatar)

## Speech-to-Speech Flow

1. Connect to WiFi using NVS credentials (one-time setup)
2. Wake word **"Hi ESP"** starts listening
3. Speech → OpenAI Whisper STT
4. Question → active Knowledge Base API (`POST /api/v1/kb/{id}/query`)
5. Answer → OpenAI TTS → speaker + OLED

**RFID:** tapping a card calls `POST /api/v1/iot/rfid/scan` and switches the active KB URL from the API response.

## Quick Start

See **[SETUP.md](./SETUP.md)** for full one-time configuration:

- WiFi SSID / Password
- OpenAI API Key
- Default Knowledge Base URL
- RFID card assignment via web dashboard

```bash
# 1. Build factory NVS (one-time credentials)
cd ../smart-learn/factory_nvs && idf.py build

# 2. Build and flash board firmware
cd ../../smart-learn-board
idf.py set-target esp32s3
idf.py build flash monitor
```

## Hardware wiring

Full pin-by-pin tables: **[WIRING.md](./WIRING.md)**

| Component | Signal | GPIO |
|-----------|--------|------|
| INMP441 | BCLK / WS / DOUT | 4 / 5 / 6 |
| MAX98357A | BCLK / LRC / DIN | 15 / 16 / 7 |
| SH1106 | SDA / SCL | 17 / 18 |
| RC522 | CS / MOSI / SCK / MISO / RST | 10 / 11 / 12 / 13 / 14 |

WiFi status is shown on the **OLED** (animated `Connecting WiFi...` → `WiFi connected` → `Say 'Hi ESP' to ask`). There is no separate WiFi LED on this board.

Change pins in `idf.py menuconfig` → **Custom ESP32-S3 Board**.

## NVS keys (one-time)

| NVS key | Setting |
|---------|---------|
| `ssid` | WiFi SSID |
| `password` | WiFi Password |
| `ChatGPT_key` | OpenAI API Key |
| `Base_url` | `https://api.openai.com/v1/` |
| `KB_url` | Default KB query URL |
| `tts_voice` | e.g. `nova` (optional) |

RFID card → KB mappings are managed via the **Smart Learn API / web dashboard** — not menuconfig.

## Project layout

```text
smart-learn-board/     # This firmware
components/bsp-custom/ # INMP441, MAX98357, SH1106, RC522 drivers
smart-learn/           # Original BOX-3 firmware + factory_nvs
```
