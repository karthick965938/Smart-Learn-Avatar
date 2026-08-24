# Smart Learn Board Firmware (ESP32-S3 Mini — Voice)

Voice firmware for **ESP32-S3 Mini**: **built-in microphone, audio amplifier, and speaker**, WiFi, wake word **"Hi Json"**, STT → KB query → TTS.

**No audio peripheral wiring** — mic, amp, and speaker are on the board.

Pairs with **[Arduino UNO Q](../../smart-learn-uno-q/README.md)** (main board) for **RC522** RFID and **OLED** display, managed via **Smart Learn Web**.

## Speech flow

1. Connect to WiFi (NVS credentials)
2. Wake word **"Hi Json"** starts listening
3. Speech → OpenAI Whisper STT
4. Question → Knowledge Base API (`POST /api/v1/kb/{id}/query` from NVS `KB_url`)
5. Answer → OpenAI TTS → built-in speaker

RFID card taps and KB selection on OLED are handled by **Arduino UNO Q** + **Smart Learn Web**.

## Quick Start

See **[SETUP.md](./SETUP.md)** for NVS (WiFi, OpenAI key, `KB_url`).

```bash
cd smart-learn-iot/smart-learn-board
idf.py set-target esp32s3
idf.py build flash monitor
```

## Hardware (ESP32-S3 Mini)

| Feature | Details |
|---------|---------|
| Microphone | **Built-in** |
| Audio amplifier | **Built-in** |
| Speaker | **Built-in** |
| WiFi | ESP32-S3 wireless |
| Wake word | **Hi Json** |
| RAG queries | KB URL from NVS |

**Wiring:** none for voice audio. Power via USB, flash firmware, speak.

RFID + OLED: **[Arduino UNO Q](../../smart-learn-uno-q/README.md)**.

> [WIRING.md](./WIRING.md) — Mini needs no wiring; optional DevKit GPIO tables only.

## NVS keys

| NVS key | Setting |
|---------|---------|
| `ssid` / `password` | WiFi |
| `ChatGPT_key` | OpenAI API key |
| `Base_url` | `https://api.openai.com/v1/` |
| `KB_url` | KB query URL for voice |
| `tts_voice` | e.g. `nova` (optional) |

RFID card → KB map: **Smart Learn Web → IoT Setup** (UNO Q scans).

## Project layout

```text
smart-learn-board/     # ESP32-S3 Mini voice firmware
smart-learn-uno-q/     # Arduino UNO Q — RC522 + OLED (main board)
```
