# 🤖 Smart Learn IoT (Firmware)

[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.3+-E7352C?style=for-the-badge&logo=espressif&logoColor=white)](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/index.html)
[![Hardware](https://img.shields.io/badge/Voice-ESP32--S3--Mini-blue?style=for-the-badge&logo=espressif)](https://www.espressif.com/en/products/socs/esp32-s3)

**Smart Learn IoT** is the **voice firmware** for Smart Learn. It runs on **ESP32-S3 Mini** with **built-in microphone, amplifier, and speaker**, WiFi, wake word **"Hi Json"**, and RAG answers via the [Smart Learn API](../smart-learn-api/README.md).

**No mic / amp / speaker wiring on the Mini** — those are on-board. Pair with **[Arduino UNO Q](../smart-learn-uno-q/README.md)** for **RC522** RFID and **OLED**, managed in [Smart Learn Web](../smart-learn-web/README.md).

[![Video Demo](https://img.shields.io/badge/Demo-Smart%20Learn-blue?style=for-the-badge&logo=youtube)](https://youtu.be/VB4N3pkbW_0)

---

## 📦 System boards

| Board | Role | Highlights |
|-------|------|------------|
| **[Arduino UNO Q](../smart-learn-uno-q/README.md)** | Main board | RC522 RFID, OLED display, Smart Learn Web integration |
| **ESP32-S3 Mini** | Voice module | **Hi Json**, STT/TTS, **built-in** mic · amp · speaker, WiFi |

Card → KB mappings: [Smart Learn Web](../smart-learn-web/README.md) **IoT Setup** (UNO Q scans).

---

## 🚀 Key Features (ESP32-S3 Mini)

- **🎙️ Hands-free speech** — Wake word **"Hi Json"**, Whisper STT, KB query, OpenAI TTS
- **🔊 Built-in audio** — On-board microphone, amplifier, and speaker (no external audio modules)
- **📚 RAG integration** — `POST /api/v1/kb/{kb_id}/query` using `KB_url` from NVS
- **🔐 NVS provisioning** — WiFi, OpenAI key, and `KB_url` stored once at flash time
- **🤝 Works with UNO Q** — RFID and OLED on the Arduino UNO Q + Smart Learn Web

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
    │  RC522 + OLED       │        │  built-in audio    │
    │  (smart-learn-uno-q)│        │  (smart-learn-board)│
    └─────────────────────┘        └────────────────────┘
```

**Arduino UNO Q (RFID + OLED):**

1. Tap card → RC522 reads UID → Python posts `POST /api/v1/iot/rfid/scan`.
2. OLED shows scan status and selected knowledge base.
3. **Smart Learn Web** → assign KB in **IoT Setup** or popup.

**ESP32-S3 Mini (voice):**

1. USB power → flash firmware + NVS (no audio wiring).
2. Say **"Hi Json"** → STT → KB query → TTS on built-in speaker.
3. Set `KB_url` in NVS to the knowledge base you want for voice.

---

## 🛠️ Hardware

### Arduino UNO Q — main board (RC522 + OLED)

Full pin tables: [`smart-learn-uno-q/README.md`](../smart-learn-uno-q/README.md)

### ESP32-S3 Mini — voice module

Firmware: [`smart-learn-board/`](./smart-learn-board/)

| Built-in | Notes |
|----------|--------|
| Microphone | On-board |
| Audio amplifier | On-board |
| Speaker | On-board |
| WiFi | ESP32-S3 wireless |
| Wake word | **Hi Json** |

**No peripheral wiring for voice.** See [WIRING.md](./smart-learn-board/WIRING.md) for Mini notes (and optional DevKit-only GPIO tables).

---

## ⚙️ Getting Started (ESP32 voice firmware)

### Prerequisites

1. **ESP-IDF v5.3+**
2. **Smart Learn API** running
3. **Arduino UNO Q** for RFID (optional for voice-only testing)

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

1. Start **Smart Learn API** and **Smart Learn Web**.
2. Deploy **Arduino UNO Q** with **RC522** + **OLED**.
3. Flash **ESP32-S3 Mini** + NVS (USB only — built-in audio).
4. Create knowledge bases in the web dashboard.
5. Tap a card on the **UNO Q** → assign KB in **IoT Setup**.
6. Say **"Hi Json"** on the **Mini** to ask questions (`KB_url` in NVS).

---

## 📁 Project Structure

```text
smart-learn-iot/
├── smart-learn-board/     # ESP32-S3 Mini voice firmware
│   ├── SETUP.md           # NVS + flash
│   ├── WIRING.md          # Mini: no wiring; DevKit optional
│   └── main/app/          # STT, TTS, WiFi voice pipeline
├── components/bsp-custom/
└── ../smart-learn-uno-q/  # Arduino UNO Q — RC522 + OLED
```

---

## 📄 License

This project is part of the Smart Learn ecosystem. See the root `README.md` for licensing information.
