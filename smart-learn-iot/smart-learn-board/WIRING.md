# Smart Learn Board — Wiring Guide

Default GPIO assignments from `components/bsp-custom/Kconfig.projbuild`.  
All modules use **3.3 V** logic. Connect **GND** common to ESP32-S3 GND.

---

## ESP32-S3 ↔ INMP441 (I2S Microphone)

| INMP441 pin | Connect to | ESP32-S3 GPIO | Notes |
|-------------|------------|---------------|-------|
| VDD | 3.3 V | 3.3 V | |
| GND | GND | GND | |
| SCK | BCLK | **GPIO 4** | Bit clock |
| WS | L/R select + WS | **GPIO 5** | Word select / left-right |
| SD | DOUT | **GPIO 6** | Serial data out → ESP32 DIN |
| L/R | GND | GND | **Left channel** (mono) |

---

## ESP32-S3 ↔ MAX98357A (I2S Amplifier / Speaker)

| MAX98357A pin | Connect to | ESP32-S3 GPIO | Notes |
|---------------|------------|---------------|-------|
| VIN | 3.3 V or 5 V | 3.3 V / 5 V | 5 V = louder |
| GND | GND | GND | |
| BCLK | BCLK | **GPIO 15** | Bit clock |
| LRC | WS | **GPIO 16** | Left/right clock |
| DIN | DOUT | **GPIO 7** | Data in ← ESP32 DOUT |
| GAIN | — | GND or float | GND = 9 dB default gain |
| SD | 3.3 V | 3.3 V | **Must be high** — chip enable |

Speaker: connect **+** and **−** to MAX98357A **+** and **−** outputs.

---

## ESP32-S3 DevKit wiring (your board)

If you use an **ESP32-S3-WROOM-1** dev board on a breadboard with a 4-pin OLED:

| OLED pin | → | ESP32-S3 |
|----------|---|----------|
| GND | → | GND |
| VCC | → | 3V3 |
| SCL | → | **GPIO 18** |
| SDA | → | **GPIO 17** |

This is the **firmware default** for `smart-learn-board`.

> Previous docs listed GPIO 8/9 — that was for a custom PCB layout.  
> DevKit users should use **GPIO 17 (SDA)** and **GPIO 18 (SCL)**.

---

## ESP32-S3 ↔ SH1106 (128×64 OLED, I2C)

| SH1106 pin | Connect to | ESP32-S3 GPIO | Notes |
|------------|------------|---------------|-------|
| VCC | 3.3 V | 3.3 V | |
| GND | GND | GND | |
| SDA | SDA | **GPIO 17** | I2C data |
| SCL | SCL | **GPIO 18** | I2C clock |

Default I2C address: **0x3C** (`CONFIG_BOARD_OLED_I2C_ADDR`).

Some modules label pins **D0 = SCL**, **D1 = SDA** — match SDA→GPIO8, SCL→GPIO9.

---

## ESP32-S3 ↔ RC522 (13.56 MHz RFID, SPI)

| RC522 pin | Connect to | ESP32-S3 GPIO | Notes |
|-----------|------------|---------------|-------|
| 3.3 V | 3.3 V | 3.3 V | **Do not use 5 V** |
| GND | GND | GND | |
| SDA | CS / NSS | **GPIO 10** | Chip select |
| SCK | SCK | **GPIO 12** | SPI clock |
| MOSI | MOSI | **GPIO 11** | Master out |
| MISO | MISO | **GPIO 13** | Master in |
| RST | Reset | **GPIO 14** | Reset |
| IRQ | — | *(not used)* | Leave unconnected |

SPI bus: **SPI2_HOST** (default).

---

## Quick reference (signal → GPIO)

| Component | Signal | GPIO |
|-----------|--------|------|
| INMP441 | BCLK | 4 |
| INMP441 | WS | 5 |
| INMP441 | DOUT | 6 |
| MAX98357A | BCLK | 15 |
| MAX98357A | LRC | 16 |
| MAX98357A | DIN | 7 |
| SH1106 | SDA | 17 |
| SH1106 | SCL | 18 |
| RC522 | CS | 10 |
| RC522 | MOSI | 11 |
| RC522 | SCK | 12 |
| RC522 | MISO | 13 |
| RC522 | RST | 14 |

---

## WiFi status on OLED

There is **no separate WiFi LED** on this board. WiFi status is shown on the **SH1106 OLED** title/status line:

| OLED text | Meaning |
|-----------|---------|
| `Connecting WiFi...` | Connecting (dots animate) |
| `WiFi connected` | Connected — then shows `Say 'Hi Json' to ask` |
| `WiFi failed` | Could not connect after retries |
| `Say 'Hi Json' to ask` | Ready for voice |

Change GPIO pins in:

```bash
idf.py menuconfig
→ Custom ESP32-S3 Board
```

---

## Power & notes

- Use a stable **3.3 V** supply for ESP32-S3; INMP441 and RC522 are 3.3 V only.
- MAX98357A can use 3.3 V or 5 V on VIN depending on your module.
- Keep I2S mic and speaker **BCLK/WS lines separate** (different GPIO sets — do not tie together).
- Add **100 nF** decoupling caps near each module VCC if the board is noisy.
