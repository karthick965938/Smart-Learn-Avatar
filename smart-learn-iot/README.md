# 🤖 Smart Learn IoT (Firmware)

[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.3+-E7352C?style=for-the-badge&logo=espressif&logoColor=white)](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/index.html)
[![Hardware](https://img.shields.io/badge/Voice-ESP32--S3--Mini-blue?style=for-the-badge&logo=espressif)](https://www.espressif.com/en/products/socs/esp32-s3)

**Smart Learn IoT** is the **voice firmware** for Smart Learn. It runs on **ESP32-S3 Mini** with built-in microphone and amplifier, external speaker, WiFi, wake word **"Hi Json"**, and RAG-powered answers via the [Smart Learn API](../smart-learn-api/README.md).

Pair with **[Arduino UNO Q](../smart-learn-uno-q/README.md)** (main board) for **RC522** RFID and **OLED** display, managed through [Smart Learn Web](../smart-learn-web/README.md).

[![Video Demo](https://img.shields.io/badge/Demo-Voice%20%2B%20RFID-blue?style=for-the-badge&logo=youtube)](https://www.youtube.com/watch?v=sbAEzvDquOA)

---

## 📦 System boards

| Board | Role | Highlights |
|-------|------|------------|
| **[Arduino UNO Q](../smart-learn-uno-q/README.md)** | Main board | RC522 RFID, OLED display, Smart Learn Web integration |
| **ESP32-S3 Mini** | Voice module | **Hi Json**, STT/TTS, built-in mic & amplifier, WiFi |

Card → KB mappings are managed in [Smart Learn Web](../smart-learn-web/README.md) **IoT Setup** (used with the UNO Q).

---

## 🚀 Key Features (ESP32-S3 Mini)

- **🎙️ Hands-free speech** — Wake word **"Hi Json"**, OpenAI Whisper STT, KB query, OpenAI TTS
- **🔊 Built-in audio** — On-board microphone and amplifier with external speaker
- **📚 RAG integration** — `POST /api/v1/kb/{kb_id}/query` using the KB URL from NVS
- **🔐 NVS provisioning** — WiFi, OpenAI key, and `KB_url` stored once at flash time
- **🤝 Works with UNO Q** — RFID and OLED on the Arduino UNO Q main board + Smart Learn Web

---

## 🏗️ Full system flow

```text
┌─────────────────────────────────────────────────────────────────┐
│                     Smart Learn Web (browser)                   │
│              KBs · IoT Setup · RFID popup · Chat                │
└────────────────────────────┬────────────────────────────────────┘
                             │ REST
                             ▼
┌─────────────────────────────────────────────────────────────────┐
│                     Smart Learn API                             │
│              RAG · rfid_cards.json · scan events                │
└──────────────▲──────────────────────────────▲───────────────────┘
               │ POST /iot/rfid/scan          │ POST /kb/{id}/query
               │                              │
    ┌──────────┴──────────┐        ┌─────────┴──────────┐
    │  Arduino UNO Q      │        │  ESP32-S3 Mini     │
    │  MAIN BOARD         │        │  VOICE MODULE      │
    │  RC522 + OLED       │        │  mic + speaker     │
    │  (smart-learn-uno-q)│        │  (smart-learn-board)│
    └─────────────────────┘        └────────────────────┘
```

**Arduino UNO Q (RFID + OLED):**

1. Tap card → RC522 reads UID → Python posts `POST /api/v1/iot/rfid/scan`.
2. OLED: **Scanning RFID…** → **Knowledge Base Selected:** + name (if assigned).
3. **Smart Learn Web** gets the scan event → assign KB in **IoT Setup** or popup → chat for assigned cards.

**ESP32-S3 Mini (voice module):**

1. Connect WiFi; use `KB_url` from NVS as the query endpoint.
2. Say **"Hi Json"** → STT → KB query → TTS on the speaker.
3. Works alongside UNO Q + Smart Learn Web for RFID-driven KB selection on the main display.

Set ESP32 `KB_url` in NVS to the knowledge base you want for voice (match the KB assigned to your RFID card on the UNO Q).

---

## 🛠️ Hardware

### Arduino UNO Q — main board (RC522 + OLED)

Project: [`../smart-learn-uno-q/`](../smart-learn-uno-q/)

**RC522 (SPI)** — use **3.3 V** power:

| RC522 | UNO Q |
|-------|-------|
| VCC | `3.3V` |
| GND | `GND` |
| RST | **D9** |
| SDA / CS | **D10** |
| MOSI | **D11** |
| MISO | **D12** |
| SCK | **D13** |

**0.96" OLED SSD1306 (I2C)** — address `0x3C`:

| OLED | UNO Q |
|------|-------|
| VCC | `3.3V` |
| GND | `GND` |
| SDA | **SDA** |
| SCL | **SCL** |

Works with **[Smart Learn Web](../smart-learn-web/README.md)** for card registration and KB assignment.

```text
Tap card → UNO Q → API → Web (IoT Setup) → assign KB → OLED shows selected KB
```

Full wiring: [`smart-learn-uno-q/README.md`](../smart-learn-uno-q/README.md)

---

### ESP32-S3 Mini — voice module

Firmware: [`smart-learn-board/`](./smart-learn-board/)

| Built-in | Notes |
|----------|--------|
| Microphone | On-board |
| Audio amplifier | On-board + **external speaker** |
| WiFi | ESP32-S3 wireless |
| Wake word | **Hi Json** → full speech-to-speech pipeline |

---

## ⚙️ Getting Started (ESP32 voice firmware)

### Prerequisites

1. **ESP-IDF v5.3+** — [Installation guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/index.html)
2. **Smart Learn API** running ([setup](../smart-learn-api/README.md))
3. **Arduino UNO Q** deployed for RFID ([`smart-learn-uno-q`](../smart-learn-uno-q/README.md))

### Build and flash

```bash
cd smart-learn-iot/smart-learn-board
idf.py set-target esp32s3
idf.py build flash monitor
```

Full guide: [`smart-learn-board/SETUP.md`](./smart-learn-board/SETUP.md)

### NVS (WiFi + OpenAI + KB URL)

```bash
curl -X POST http://<api-host>:5000/api/v1/iot/generate-nvs \
  -H "Content-Type: application/json" \
  -d '{
    "ssid": "YourWiFi",
    "password": "your-password",
    "openai_key": "sk-your-openai-key",
    "base_url": "https://api.openai.com/v1/",
    "kb_url": "http://<api-host>:5000/api/v1/kb/<kb_id>/query",
    "tts_voice": "nova",
    "theme": "dark"
  }' \
  --output nvs.bin
```

Flash `nvs.bin` at `0x9000`, then flash the main firmware.

---

## 🕹️ End-to-End Usage

1. Start **Smart Learn API** and **Smart Learn Web** on your PC.
2. Deploy **Arduino UNO Q** with **RC522** + **OLED** ([`smart-learn-uno-q`](../smart-learn-uno-q/)).
3. Flash **ESP32-S3 Mini** voice firmware + NVS; connect external speaker for **Hi Json** voice Q&A.
4. Create knowledge bases and upload documents in the web dashboard.
5. Tap a card on the **UNO Q** → assign KB in **IoT Setup** or the scan popup.
6. UNO Q OLED shows **Knowledge Base Selected**; web chat opens for assigned cards.
7. Say **"Hi Json"** on the **ESP32** to ask questions (uses `KB_url` from NVS).

---

## 📁 Project Structure

```text
smart-learn-iot/
├── smart-learn-board/     # ESP32-S3 Mini voice firmware
│   ├── main/app/          # STT, TTS, WiFi voice pipeline
│   └── SETUP.md
├── components/bsp-custom/
└── ../smart-learn-uno-q/  # Arduino UNO Q — main board (RC522 + OLED)
```

---

## 📄 License

This project is part of the Smart Learn ecosystem. See the root `README.md` for licensing information.
