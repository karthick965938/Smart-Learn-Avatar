#include <Arduino_RouterBridge.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include "ssd1306_i2c.h"
#include <map>

// ---------------------------------------------------------------------------
// Hardware — 0.96" OLED (SSD1306, 128×64, I2C)
// Uses a Zephyr-safe Wire driver (Adafruit SSD1306 does not build on UNO Q).
// ---------------------------------------------------------------------------

#define SS_PIN 10
#define RST_PIN 9
#define OLED_I2C_ADDRESS 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_LINE_CHARS 21

MFRC522 rfid(SS_PIN, RST_PIN);
Ssd1306I2c display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ---------------------------------------------------------------------------
// Scan state
// ---------------------------------------------------------------------------

String lastUID = "";
unsigned long lastCallTime = 0;
const unsigned long CALL_COOLDOWN_MS = 3000;
const unsigned long KB_RESULT_DISPLAY_MS = 10000;
const unsigned long STATUS_DISPLAY_MS = 5000;
const unsigned long DUPLICATE_DISPLAY_MS = 2000;

// ---------------------------------------------------------------------------
// OLED helpers
// ---------------------------------------------------------------------------

String truncateText(const String &text, int maxLen = OLED_LINE_CHARS) {
  if ((int)text.length() <= maxLen) {
    return text;
  }
  return text.substring(0, maxLen - 1) + "~";
}

void wrapText(const String &text, String &line1, String &line2) {
  if ((int)text.length() <= OLED_LINE_CHARS) {
    line1 = text;
    line2 = "";
    return;
  }

  int breakAt = OLED_LINE_CHARS;
  int lastSpace = text.lastIndexOf(' ', OLED_LINE_CHARS);
  if (lastSpace > 0) {
    breakAt = lastSpace;
  }

  line1 = text.substring(0, breakAt);
  line2 = truncateText(text.substring(breakAt));
  line2.trim();
}

void oledShow(const char *line1, const char *line2 = "", const char *line3 = "") {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.println("Smart Learn");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  int y = 16;
  const char *lines[] = {line1, line2, line3};

  for (int i = 0; i < 3; i++) {
    if (lines[i] && lines[i][0]) {
      display.setCursor(0, y);
      display.println(lines[i]);
      y += 12;
    }
  }

  display.display();
}

void oledShowReady() {
  oledShow("Tap your card", "to select a", "knowledge base");
}

void oledShowStarting() {
  oledShow("Starting up...", "Connecting RFID", "reader");
}

void oledShowScanning() {
  oledShow("Reading card...", "Please wait");
}

void oledShowAlreadyScanned() {
  oledShow("Card detected", "Remove card to", "scan again");
}

void oledShowConnectionFailed() {
  oledShow("Server unreachable", "Check API and", "network connection");
}

void oledShowUnassigned() {
  oledShow("New card found", "Assign this card", "in Smart Learn Web");
}

void oledShowKbSelected(const String &kbName) {
  String nameLine1;
  String nameLine2;
  wrapText(kbName, nameLine1, nameLine2);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.println("Smart Learn");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  display.setCursor(0, 16);
  display.println("Knowledge base:");
  display.setCursor(0, 30);
  display.println(nameLine1.c_str());

  if (nameLine2.length() > 0) {
    display.setCursor(0, 44);
    display.println(nameLine2.c_str());
  }

  display.display();
}

// ---------------------------------------------------------------------------
// RFID helpers
// ---------------------------------------------------------------------------

bool readCardUid(String &uid, String &displayUid) {
  uid = "";
  displayUid = "";

  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) {
      uid += "0";
      displayUid += "0";
    }

    String byteString = String(rfid.uid.uidByte[i], HEX);
    uid += byteString;
    displayUid += byteString;

    if (i < rfid.uid.size - 1) {
      displayUid += ":";
    }
  }

  uid.toUpperCase();
  displayUid.toUpperCase();
  return uid.length() > 0;
}

void haltCard() {
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

bool isCooldownActive(const String &uid) {
  unsigned long now = millis();
  return uid == lastUID && (now - lastCallTime) <= CALL_COOLDOWN_MS;
}

void rememberScan(const String &uid) {
  lastUID = uid;
  lastCallTime = millis();
}

unsigned long handleScanResult(const std::map<String, String> &result) {
  bool ok = result.count("ok") && result.at("ok") == "1";
  bool assigned = result.count("assigned") && result.at("assigned") == "1";
  String kbName = result.count("kb_name") ? result.at("kb_name") : "";

  if (!ok) {
    oledShowConnectionFailed();
    return STATUS_DISPLAY_MS;
  }

  if (assigned && kbName.length() > 0) {
    oledShowKbSelected(kbName);
    Monitor.print("Knowledge base selected: ");
    Monitor.println(kbName);
    return KB_RESULT_DISPLAY_MS;
  }

  oledShowUnassigned();
  Monitor.println("Card scanned but not assigned.");
  return STATUS_DISPLAY_MS;
}

// ---------------------------------------------------------------------------
// Setup / loop
// ---------------------------------------------------------------------------

void setup() {
  Monitor.begin(9600);
  Bridge.begin();

  Wire.begin();
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS)) {
    Monitor.println("0.96 OLED (SSD1306) initialization failed!");
  } else {
    display.clearDisplay();
    display.display();
    oledShowStarting();
    delay(1500);
  }

  SPI.begin();
  rfid.PCD_Init();
  delay(100);

  Monitor.println("Smart Learn RFID ready.");
  oledShowReady();
}

void showMessageThenReady(unsigned long displayMs) {
  delay(displayMs);
  oledShowReady();
}

void loop() {
  if (!rfid.PICC_IsNewCardPresent()) {
    delay(100);
    return;
  }

  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }

  String uid;
  String displayUid;
  if (!readCardUid(uid, displayUid)) {
    haltCard();
    return;
  }

  Monitor.print("RFID detected: ");
  Monitor.println(uid);

  if (isCooldownActive(uid)) {
    Monitor.println("Same card - scan skipped.");
    oledShowAlreadyScanned();
    haltCard();
    showMessageThenReady(DUPLICATE_DISPLAY_MS);
    return;
  }

  oledShowScanning();
  Monitor.println("Calling API...");

  std::map<String, String> result;
  bool bridgeOk = Bridge.call("rfid_detected", uid).result(result);

  unsigned long displayMs = STATUS_DISPLAY_MS;
  if (!bridgeOk) {
    Monitor.println("Bridge call failed.");
    oledShowConnectionFailed();
  } else {
    displayMs = handleScanResult(result);
  }

  rememberScan(uid);
  haltCard();
  showMessageThenReady(displayMs);
}
