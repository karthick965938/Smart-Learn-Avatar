# Smart Learn Board — Setup Guide

One-time device configuration and speech-to-speech flow for the **ESP32-S3 + RC522** board firmware (`smart-learn-board`).

---

## Speech-to-Speech Flow

```text
1. Boot → read WiFi / OpenAI / default KB URL from NVS
2. Connect WiFi
3. Wait for wake word "Hi ESP"
4. Record speech → OpenAI Whisper (STT)
5. POST question to active Knowledge Base URL
6. OpenAI TTS (tts-1) → speaker playback
7. Show question + answer on OLED
```

**RFID card tap (runtime):**

```text
1. RC522 reads card UID
2. Device POSTs /api/v1/iot/rfid/scan { "uid": "..." }
3. API returns { assigned, kb_id, kb_name, kb_url }
4. If assigned  → switch active KB URL for voice queries
5. If unassigned → keep default KB URL from NVS
6. Web dashboard shows popup to assign a Knowledge Base
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

> **Default KB URL** is used at boot and when an unassigned RFID card is scanned.  
> **Assigned RFID cards** override the active KB URL dynamically via the API — no reflash needed.

---

## Setup Method A — menuconfig + factory_nvs (recommended)

### Step 1 — Configure defaults

```bash
cd smart-learn-iot/smart-learn/factory_nvs
idf.py set-target esp32s3
idf.py menuconfig
```

Under **Example Configuration**, set (defaults are pre-filled — verify and update **Knowledge Base URL** with your KB id):

| menuconfig field | Default value | Maps to NVS key |
|------------------|---------------|-----------------|
| WiFi SSID | `FTTH-F8D0` | `ssid` |
| WiFi Password | `12345678` | `password` |
| OpenAI Key | *(pre-filled)* | `ChatGPT_key` |
| Base URL | `https://api.openai.com/v1/` | `Base_url` |
| Knowledge Base URL | `http://172.22.200.239:5000/api/v1/kb/xxxxxxxx/query` | `KB_url` |
| TTS Voice Selection | `shimmer` | `tts_voice` |

**Knowledge Base URL format:**

```text
http://<your-pc-ip>:8000/api/v1/kb/<kb_id>/query
```

Example:

```text
http://172.22.200.239:5000/api/v1/kb/a1b2c3d4/query
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

On first boot without valid NVS, the device enters UF2 mode. Connect USB and edit **CONFIG.INI** on the `ESP-Box` drive:

```ini
ssid=FTTH-F8D0
password=12345678
ChatGPT_key=sk-your-openai-key
Base_url=https://api.openai.com/v1/
KB_url=http://172.22.200.239:5000/api/v1/kb/a1b2c3d4/query
tts_voice=nova
```

Save and reboot.

---

## Setup Method B — API NVS generator

Generate a 16 KB NVS binary from the Smart Learn API:

```bash
curl -X POST http://172.22.200.239:5000/api/v1/iot/generate-nvs \
  -H "Content-Type: application/json" \
  -d '{
    "ssid": "FTTH-F8D0",
    "password": "12345678",
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

## API + RFID Setup

### 1. Start the API

```bash
cd smart-learn-api
```

Add to `.env`:

```env
OPENAI_API_KEY=sk-your-key
API_BASE_URL=http://172.22.200.239:5000
```

> `API_BASE_URL` must match the host in your NVS `KB_url` so RFID scan responses return correct `kb_url` values.

```bash
uvicorn app.main:app --host 0.0.0.0 --port 8000
```

### 2. Create a Knowledge Base

```bash
curl -X POST http://172.22.200.239:5000/api/v1/kbs \
  -H "Content-Type: application/json" \
  -d '{"name": "My KB"}'
```

Use the returned `id` in your NVS `KB_url`.

### 3. Assign RFID cards (web dashboard)

1. Open the Smart Learn web dashboard
2. Scan an RFID card on the device
3. A popup appears — assign a Knowledge Base
4. Next scan switches the device to that KB automatically

Or via API:

```bash
curl -X POST http://172.22.200.239:5000/api/v1/iot/rfid/cards \
  -H "Content-Type: application/json" \
  -d '{"uid": "12344", "kb_id": "a1b2c3d4"}'
```

### 4. Verify RFID scan from device

When a card is tapped, the device calls:

```bash
POST /api/v1/iot/rfid/scan
{ "uid": "12344" }
```

Response (assigned):

```json
{
  "id": 1,
  "uid": "12344",
  "assigned": true,
  "kb_id": "a1b2c3d4",
  "kb_name": "My KB",
  "kb_url": "http://172.22.200.239:5000/api/v1/kb/a1b2c3d4/query",
  "timestamp": 1787071700
}
```

The firmware updates the active KB URL from `kb_url` in this response.

---

## Testing Speech-to-Speech

1. Ensure API is running and reachable from the device over WiFi
2. Ensure default `KB_url` in NVS points to a valid KB with documents
3. Flash firmware and open serial monitor:

```bash
idf.py monitor
```

4. Say **"Hi ESP"**, then ask a question
5. Watch serial logs:

```text
I (xxx) settings: Default KB URL: http://...
I (xxx) app_rfid: RFID scan API HTTP 200
I (xxx) settings: Active KB switched to My KB (http://...)
```

---

## Configuration Summary

| What | Where set | Changes at runtime? |
|------|-----------|---------------------|
| WiFi SSID / Password | NVS (one-time) | No |
| OpenAI API Key | NVS (one-time) | No |
| OpenAI Base URL | NVS (one-time) | No |
| Default KB URL | NVS (one-time) | No (fallback only) |
| Active KB URL | API via RFID scan | **Yes — per card tap** |
| RFID card → KB map | Web dashboard / API | **Yes — no reflash** |

---

## Troubleshooting

| Problem | Check |
|---------|-------|
| Device reboots to UF2 | NVS keys missing — run factory_nvs setup |
| STT fails | `ChatGPT_key` and `Base_url` in NVS |
| KB query fails | `KB_url` reachable from device; API running |
| RFID doesn't switch KB | Card assigned in web? API `API_BASE_URL` matches NVS host? |
| RFID scan HTTP fails | Device on same network as API; `KB_url` uses LAN IP not `localhost` |

---

## Key Source Files

| File | Purpose |
|------|---------|
| `main/main.c` | STT → KB query → TTS pipeline |
| `main/settings/settings.c` | NVS read + active KB URL management |
| `main/app/app_rfid.c` | RFID scan → API → dynamic KB switch |
| `main/app/app_sr.c` | Wake word "Hi ESP" |
| `main/app/app_audio.c` | Record / playback |
| `../smart-learn/factory_nvs/` | One-time NVS provisioning |

See also: [API.md](../../smart-learn-api/API.md) for full API reference.
