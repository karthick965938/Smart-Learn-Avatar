# Smart Learn Board — Setup Guide

One-time configuration and speech-to-speech flow for **ESP32-S3 Mini** voice firmware (`smart-learn-board`).

**Board:** ESP32-S3 Mini with **built-in microphone, amplifier, and speaker** — no external audio wiring.  
**RFID / OLED:** use **[Arduino UNO Q](../../smart-learn-uno-q/README.md)**, not the ESP32.

Hardware notes: [WIRING.md](./WIRING.md)

---

## Speech-to-Speech Flow (ESP32-S3 Mini)

```text
1. Boot → read WiFi / OpenAI / default KB URL from NVS
2. Connect WiFi
3. Wait for wake word "Hi Json"
4. Record speech (built-in mic) → OpenAI Whisper (STT)
5. POST question to Knowledge Base URL from NVS
6. OpenAI TTS (tts-1) → built-in speaker
```

**RFID / KB selection** happens on the **Arduino UNO Q** + **Smart Learn Web** (not on the Mini):

```text
1. Tap card on UNO Q RC522
2. UNO Q POSTs /api/v1/iot/rfid/scan
3. Assign KB in Smart Learn Web (IoT Setup / popup)
4. Set ESP32 NVS KB_url to the same knowledge base for voice
```

---

## One-Time Configuration (NVS)

These values are written **once** to the NVS partition (`0x9000`, namespace `configuration`) and persist across reboots.

| Setting | NVS key | Example |
|---------|---------|---------|
| **WiFi SSID** | `ssid` | `MyHomeWiFi` |
| **WiFi Password** | `password` | `your-wifi-password` |
| **OpenAI API Key** | `ChatGPT_key` | `sk-...` |
| **OpenAI Base URL** | `Base_url` | `https://api.openai.com/v1/` |
| **Default Knowledge Base URL** | `KB_url` | `http://172.22.200.239:5000/api/v1/kb/{kb_id}/query` |
| TTS voice (optional) | `tts_voice` | `nova` |

> **`KB_url`** is the knowledge base the Mini uses for voice answers. Align it with the KB you assign to RFID cards on the UNO Q.

---

## Setup Method A — menuconfig + factory_nvs (recommended)

### Step 1 — Configure defaults

```bash
cd smart-learn-iot/smart-learn/factory_nvs
idf.py set-target esp32s3
idf.py menuconfig
```

Under **Example Configuration**, set (verify and update **Knowledge Base URL** with your KB id):

| menuconfig field | Maps to NVS key |
|------------------|-----------------|
| WiFi SSID | `ssid` |
| WiFi Password | `password` |
| OpenAI Key | `ChatGPT_key` |
| Base URL | `Base_url` |
| Knowledge Base URL | `KB_url` |
| TTS Voice Selection | `tts_voice` |

**Knowledge Base URL format:**

```text
http://<your-pc-ip>:5000/api/v1/kb/<kb_id>/query
```

Get `<kb_id>` from the web dashboard or `GET /api/v1/kbs`.

### Step 2 — Build factory NVS

```bash
idf.py build
```

### Step 3 — Build and flash main firmware

```bash
cd ../../smart-learn-board
idf.py set-target esp32s3
idf.py build flash monitor
```

The build copies `factory_nvs.bin` into the firmware image automatically.

### Step 4 — Edit via USB (optional)

On first boot without valid NVS, the device may enter UF2 mode. Connect USB and edit **CONFIG.INI** on the drive:

```ini
ssid=YourWiFi
password=your-password
ChatGPT_key=sk-your-openai-key
Base_url=https://api.openai.com/v1/
KB_url=http://172.22.200.239:5000/api/v1/kb/a1b2c3d4/query
tts_voice=nova
```

Save and reboot.

---

## Setup Method B — API NVS generator

```bash
curl -X POST http://172.22.200.239:5000/api/v1/iot/generate-nvs \
  -H "Content-Type: application/json" \
  -d '{
    "ssid": "YourWiFi",
    "password": "your-password",
    "openai_key": "sk-your-openai-key",
    "base_url": "https://api.openai.com/v1/",
    "kb_url": "http://172.22.200.239:5000/api/v1/kb/a1b2c3d4/query",
    "tts_voice": "nova",
    "theme": "light"
  }' \
  --output nvs.bin
```

Flash `nvs.bin` at partition offset `0x9000`.

---

## API + RFID (via UNO Q)

### 1. Start the API

```bash
cd smart-learn-api
```

Add to `.env`:

```env
OPENAI_API_KEY=sk-your-key
API_BASE_URL=http://172.22.200.239:5000
```

```bash
uvicorn app.main:app --host 0.0.0.0 --port 5000
```

### 2. Create a Knowledge Base

```bash
curl -X POST http://172.22.200.239:5000/api/v1/kbs \
  -H "Content-Type: application/json" \
  -d '{"name": "My KB"}'
```

Use the returned `id` in your ESP32 NVS `KB_url`.

### 3. Assign RFID cards (UNO Q + web)

1. Open **Smart Learn Web**
2. Tap a card on the **Arduino UNO Q**
3. Assign a Knowledge Base in the popup / **IoT Setup**
4. Keep ESP32 `KB_url` pointed at the KB you want for voice

---

## Testing Speech-to-Speech

1. API reachable from the Mini over WiFi  
2. `KB_url` in NVS points to a KB with documents  
3. Flash and monitor:

```bash
idf.py monitor
```

4. Say **"Hi Json"**, then ask a question  

---

## Configuration Summary

| What | Where set | Changes at runtime? |
|------|-----------|---------------------|
| WiFi SSID / Password | NVS (one-time) | No |
| OpenAI API Key | NVS (one-time) | No |
| OpenAI Base URL | NVS (one-time) | No |
| Voice KB URL | NVS (`KB_url`) | Reflash / re-provision NVS to change |
| RFID card → KB map | Web + UNO Q | Yes — no ESP32 reflash |

---

## Troubleshooting

| Problem | Check |
|---------|-------|
| Device reboots to UF2 | NVS keys missing — run factory_nvs setup |
| STT fails | `ChatGPT_key` and `Base_url` in NVS; built-in mic unobstructed |
| No TTS audio | Built-in speaker path; volume / power |
| KB query fails | `KB_url` reachable from device; API running; use LAN IP not `localhost` |
| RFID / OLED | Wire and use **UNO Q**, not the Mini — see [UNO Q README](../../smart-learn-uno-q/README.md) |

---

## Key Source Files

| File | Purpose |
|------|---------|
| `main/main.c` | STT → KB query → TTS pipeline |
| `main/settings/settings.c` | NVS read + KB URL |
| `main/app/app_sr.c` | Wake word "Hi Json" |
| `main/app/app_audio.c` | Record / playback (on-board audio) |
| `../smart-learn/factory_nvs/` | One-time NVS provisioning |

See also: [API.md](../../smart-learn-api/API.md)
