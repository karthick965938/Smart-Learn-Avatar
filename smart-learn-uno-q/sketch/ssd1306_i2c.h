#pragma once

#include <Adafruit_GFX.h>
#include <Wire.h>
#include <string.h>

// Minimal SSD1306 I2C driver for Arduino UNO Q (Zephyr).
// Avoids Adafruit_SSD1306, which fails to compile on ARDUINO_ARCH_ZEPHYR
// because of incompatible portOutputRegister / digitalPinToPort macros.

#define SSD1306_WHITE 1
#define SSD1306_BLACK 0
#define SSD1306_SWITCHCAPVCC 0x02

class Ssd1306I2c : public Adafruit_GFX {
public:
  Ssd1306I2c(uint8_t width, uint8_t height, TwoWire *wire = &Wire, int8_t /*rst*/ = -1)
      : Adafruit_GFX(width, height), _wire(wire), _addr(0x3C) {
    memset(_buffer, 0, sizeof(_buffer));
  }

  bool begin(uint8_t /*vccstate*/ = SSD1306_SWITCHCAPVCC, uint8_t address = 0x3C) {
    _addr = address;
    _wire->begin();

    static const uint8_t initCmds[] = {
        0xAE,        // display off
        0xD5, 0x80,  // clock div
        0xA8, 0x3F,  // multiplex 64
        0xD3, 0x00,  // display offset
        0x40,        // start line
        0x8D, 0x14,  // charge pump on
        0x20, 0x00,  // horizontal addressing
        0xA1,        // segment remap
        0xC8,        // COM scan dec
        0xDA, 0x12,  // COM pins
        0x81, 0xCF,  // contrast
        0xD9, 0xF1,  // precharge
        0xDB, 0x40,  // vcom detect
        0xA4,        // resume RAM
        0xA6,        // normal display
        0xAF,        // display on
    };

    for (uint8_t i = 0; i < sizeof(initCmds); i++) {
      if (!sendCommand(initCmds[i])) {
        return false;
      }
    }

    clearDisplay();
    display();
    return true;
  }

  void clearDisplay() {
    memset(_buffer, 0, sizeof(_buffer));
  }

  void display() {
    sendCommand(0x21);  // column addr
    sendCommand(0);
    sendCommand(WIDTH - 1);
    sendCommand(0x22);  // page addr
    sendCommand(0);
    sendCommand((HEIGHT / 8) - 1);

    for (uint16_t i = 0; i < sizeof(_buffer);) {
      _wire->beginTransmission(_addr);
      _wire->write(0x40);  // data mode
      for (uint8_t n = 0; n < 16 && i < sizeof(_buffer); n++, i++) {
        _wire->write(_buffer[i]);
      }
      _wire->endTransmission();
    }
  }

  void drawPixel(int16_t x, int16_t y, uint16_t color) override {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) {
      return;
    }

    const uint16_t index = x + (y / 8) * WIDTH;
    const uint8_t bit = 1 << (y & 7);

    if (color) {
      _buffer[index] |= bit;
    } else {
      _buffer[index] &= ~bit;
    }
  }

private:
  bool sendCommand(uint8_t cmd) {
    _wire->beginTransmission(_addr);
    _wire->write(0x00);  // command mode
    _wire->write(cmd);
    return _wire->endTransmission() == 0;
  }

  TwoWire *_wire;
  uint8_t _addr;
  uint8_t _buffer[128 * 64 / 8];
};
