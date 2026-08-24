# Smart Learn Board — Hardware Notes

## Supported board for this project: ESP32-S3 Mini

**Smart Learn voice firmware** targets the **ESP32-S3 Mini**.

| On the ESP32-S3 Mini | Status |
|----------------------|--------|
| Microphone | **Built-in** — no external mic wiring |
| Audio amplifier | **Built-in** — no external amp wiring |
| Speaker | **Built-in** — no external speaker wiring |
| WiFi | On-chip |

**You do not need to wire INMP441, MAX98357A, OLED, or RC522 to the ESP32-S3 Mini.**

RFID + OLED belong on the **[Arduino UNO Q](../../smart-learn-uno-q/README.md)** main board. See that README for UNO Q wiring (RC522 SPI + OLED I2C).

### What to connect on ESP32-S3 Mini

1. USB power / flash cable  
2. Flash firmware + NVS ([SETUP.md](./SETUP.md))  
3. Say **"Hi Json"** to start voice Q&A  

That’s it for the voice module.

---

## Board roles (Smart Learn)

| Board | Role | Peripherals |
|-------|------|-------------|
| **ESP32-S3 Mini** | Voice module | Built-in mic, amp, speaker |
| **Arduino UNO Q** | Main board | RC522 + OLED (wired to UNO Q) |

---

## Optional: ESP32-S3 DevKit only (not Mini)

> **Skip this section if you use ESP32-S3 Mini.**  
> The GPIO tables below are for an **ESP32-S3 DevKit / WROOM breadboard** build with external modules. They are **not** required for the Smart Learn ESP32-S3 Mini setup.

Default GPIOs come from `components/bsp-custom/Kconfig.projbuild`. Use **3.3 V** logic and a common **GND**.

### External I2S mic (INMP441) — DevKit only

| INMP441 | ESP32-S3 GPIO |
|---------|---------------|
| VDD | 3.3 V |
| GND | GND |
| SCK | GPIO 4 |
| WS | GPIO 5 |
| SD | GPIO 6 |
| L/R | GND (left) |

### External I2S amp (MAX98357A) — DevKit only

| MAX98357A | ESP32-S3 GPIO |
|-----------|---------------|
| VIN | 3.3 V or 5 V |
| GND | GND |
| BCLK | GPIO 15 |
| LRC | GPIO 16 |
| DIN | GPIO 7 |
| SD | 3.3 V (enable) |

### External OLED / RC522 — DevKit only

In the Smart Learn product setup, use **UNO Q** for these instead. DevKit GPIO defaults (legacy):

| Module | Signal | GPIO |
|--------|--------|------|
| OLED (I2C) | SDA / SCL | 17 / 18 |
| RC522 (SPI) | CS / MOSI / SCK / MISO / RST | 10 / 11 / 12 / 13 / 14 |

Change pins via:

```bash
idf.py menuconfig
→ Custom ESP32-S3 Board
```

---

## Related docs

- [SETUP.md](./SETUP.md) — NVS, flash, voice flow (ESP32-S3 Mini)
- [README.md](./README.md) — voice firmware overview
- [UNO Q wiring](../../smart-learn-uno-q/README.md) — RC522 + OLED
