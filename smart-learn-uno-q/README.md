# Smart Learn 2.0 — Arduino UNO Q

RFID card scanner for the Smart Learn Avatar platform. Reads an MFRC522 tag, posts the UID to the API, and shows the result on a 128×64 OLED.

## Flow

1. **Ready** — `Tap your card`
2. **Scanning** — `Scanning RFID...` + UID
3. **Assigned** — `Knowledge Base Selected:` + KB name from API
4. **Unassigned** — `Card not assigned` / assign in web app
5. **Error** — `Connection failed` / `Check server`

## Config

Edit `python/main.py`:

```python
API_URL = "http://YOUR_HOST:5000/api/v1/iot/rfid/scan"
```

## Wiring

| Module   | UNO Q pin |
|----------|-----------|
| RC522 SS | 10        |
| RC522 RST| 9         |
| OLED SDA | I2C SDA   |
| OLED SCL | I2C SCL   |

OLED I2C address: `0x3C`
