# Smart Learn 2.0 — Arduino UNO Q (Main Board)

**Arduino UNO Q** is the **main board** for Smart Learn. It connects **RC522** (RFID) and **SH1106 OLED**, posts card scans to the API, and works with **[Smart Learn Web](../smart-learn-web/README.md)** for knowledge base assignment.

Works together with **ESP32-S3 Mini** for **Hi Json** voice Q&A (built-in mic, amplifier, and speaker).

## System layout

```text
┌─────────────────────┐     ┌─────────────────────┐
│   Arduino UNO Q     │     │   ESP32-S3 Mini     │
│   MAIN BOARD        │     │   VOICE MODULE      │
│   RC522 + OLED      │     │   Hi Json · mic ·   │
│                     │     │   amplifier · WiFi  │
└──────────┬──────────┘     └──────────┬──────────┘
           │ POST /iot/rfid/scan       │ POST /kb/.../query
           └────────────┬──────────────┘
                        ▼
              Smart Learn API  ◄──  Smart Learn Web
```

## How it fits together

```text
UNO Q (RC522 + OLED)  ──POST /iot/rfid/scan──►  Smart Learn API
                                                        ▲
Smart Learn Web (browser)  ◄── poll events ────────────┘
       │
       └── IoT Setup / scan popup → assign card to Knowledge Base
```

1. Run **Smart Learn API** and **Smart Learn Web** on your PC.
2. Wire **RC522** and **OLED** to the **Arduino UNO Q**; flash sketch + Python.
3. Tap a card → UID is sent to the API.
4. In the web app: **IoT Setup** or **RFID scan popup** → assign a Knowledge Base.
5. Tap again → OLED shows **Knowledge Base Selected:** + name.

## OLED flow

1. **Ready** — `Tap your card`
2. **Scanning** — `Scanning RFID...` + UID
3. **Assigned** — `Knowledge Base Selected:` + KB name from API
4. **Unassigned** — `Card not assigned` / assign in Smart Learn Web
5. **Error** — `Connection failed` / `Check server`

## Config

Edit `python/main.py`:

```python
API_URL = "http://YOUR_HOST:5000/api/v1/iot/rfid/scan"
```

Use the same host as `VITE_API_BASE_URL` (web) and `API_BASE_URL` (API).

## Wiring

| Module   | UNO Q pin |
|----------|-----------|
| RC522 SS | 10        |
| RC522 RST| 9         |
| OLED SDA | I2C SDA   |
| OLED SCL | I2C SCL   |

OLED I2C address: `0x3C`

## Related docs

- [Smart Learn Web — IoT Setup](../smart-learn-web/README.md#iot-setup-rfid-cards)
- [Smart Learn IoT — ESP32 voice firmware](../smart-learn-iot/README.md)
