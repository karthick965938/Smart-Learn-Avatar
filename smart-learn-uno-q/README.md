# Smart Learn 2.0 — Arduino UNO Q (Main Board)

**Arduino UNO Q** is the **main board** for Smart Learn. It connects **RC522** (RFID) and a **0.96" SSD1306 OLED**, posts card scans to the API, and works with **[Smart Learn Web](../smart-learn-web/README.md)** for knowledge base assignment.

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

1. **Idle** — `Tap your card` / `to select a` / `knowledge base`
2. **Scanning** — `Reading card...` / `Please wait`
3. **Assigned** — `Knowledge base:` + name (shown for **10 seconds**), then returns to idle
4. **Unassigned** — `New card found` / assign in Smart Learn Web (5 seconds, then idle)
5. **Error** — `Server unreachable` / check API and network (5 seconds, then idle)

Header on all screens: **Smart Learn**

## Config

Edit `python/main.py`:

```python
API_URL = "http://YOUR_HOST:5000/api/v1/iot/rfid/scan"
```

Use the same host as `VITE_API_BASE_URL` (web) and `API_BASE_URL` (API).

## Wiring

Power both modules from **3.3 V** (RC522 is 3.3 V only). Share a common **GND** with the UNO Q.

### RC522 (RFID) — SPI

| RC522 pin | Connect to | UNO Q pin | Notes |
|-----------|------------|-----------|--------|
| **VCC** | 3.3 V | `3.3V` | Do not use 5 V |
| **GND** | Ground | `GND` | Common ground |
| **RST** | Reset | **D9** | Soft reset (`RST_PIN`) |
| **SDA / NSS / CS** | Chip select | **D10** | SPI SS (`SS_PIN`) |
| **MOSI** | SPI data out | **D11** | SPI MOSI |
| **MISO** | SPI data in | **D12** | SPI MISO |
| **SCK** | SPI clock | **D13** | SPI SCK |
| **IRQ** | — | *not used* | Leave unconnected |

Some RC522 boards label chip select as **SDA** — that is the SPI CS line, not I2C.

### 0.96" OLED (SSD1306, 128×64) — I2C

| OLED pin | Connect to | UNO Q pin | Notes |
|----------|------------|-----------|--------|
| **VCC** | 3.3 V | `3.3V` | Or `5V` if your module supports it |
| **GND** | Ground | `GND` | Common ground |
| **SDA** | I2C data | **D20** (SDA) | `Wire` default |
| **SCL** | I2C clock | **D21** (SCL) | `Wire` default |

I2C address: **`0x3C`** (`OLED_I2C_ADDRESS` in the sketch). Driver: **Adafruit SSD1306**.

Some OLED modules label pins **D0 = SCL**, **D1 = SDA** — match those to **D21** and **D20**.

### Quick pin summary

```text
RC522          UNO Q              0.96" OLED      UNO Q
─────          ─────              ──────────      ─────
VCC   ──────►  3.3V               VCC   ──────►  3.3V
GND   ──────►  GND                GND   ──────►  GND
RST   ──────►  D9                 SDA   ──────►  D20
SDA/CS──────►  D10                SCL   ──────►  D21
MOSI  ──────►  D11
MISO  ──────►  D12
SCK   ──────►  D13
```

## Related docs

- [Smart Learn Web — IoT Setup](../smart-learn-web/README.md#iot-setup-rfid-cards)
- [Smart Learn IoT — ESP32 voice firmware](../smart-learn-iot/README.md)
