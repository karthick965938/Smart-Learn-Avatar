#include <Arduino_RouterBridge.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <map>

// ---------------------------------------------------------------------------
// Hardware
// ---------------------------------------------------------------------------

#define SS_PIN 10
#define RST_PIN 9
#define OLED_I2C_ADDRESS 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_LINE_CHARS 21

MFRC522 rfid(SS_PIN, RST_PIN);
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ---------------------------------------------------------------------------
// Scan state
// ---------------------------------------------------------------------------

String lastUID = "";
unsigned long lastCallTime = 0;
const unsigned long CALL_COOLDOWN_MS = 3000;

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
  display.setTextColor(SH110X_WHITE);

  display.setCursor(0, 0);
  display.println("Smart Learn 2.0");
  display.drawLine(0, 10, 127, 10, SH110X_WHITE);

  display.setCursor(0, 14);
  if (line1 && line1[0]) {
    display.println(line1);
  }

  display.setCursor(0, 26);
  if (line2 && line2[0]) {
    display.println(line2);
  }

  display.setCursor(0, 38);
  if (line3 && line3[0]) {
    display.println(line3);
  }

  display.display();
}

void oledShowReady() {
  oledShow("Ready", "Tap your card");
}

void oledShowStarting() {
  oledShow("Starting...", "RFID reader");
}

void oledShowScanning(const String &displayUid) {
  String uidLine = "UID: " + displayUid;
  oledShow("Scanning RFID...", uidLine.c_str());
}

void oledShowAlreadyScanned() {
  oledShow("Already scanned", "Remove card");
}

void oledShowConnectionFailed() {
  oledShow("Connection failed", "Check server");
}

void oledShowUnassigned() {
  oledShow("Card not assigned", "Assign in web app");
}

void oledShowKbSelected(const String &kbName) {
  String nameLine1;
  String nameLine2;
  wrapText(kbName, nameLine1, nameLine2);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);

  display.setCursor(0, 0);
  display.println("Smart Learn 2.0");
  display.drawLine(0, 10, 127, 10, SH110X_WHITE);

  display.setCursor(0, 14);
  display.println("Knowledge Base");
  display.setCursor(0, 26);
  display.println("Selected:");
  display.setCursor(0, 38);
  display.println(nameLine1.c_str());

  if (nameLine2.length() > 0) {
    display.setCursor(0, 50);
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

void handleScanResult(const std::map<String, String> &result) {
  bool ok = result.count("ok") && result.at("ok") == "1";
  bool assigned = result.count("assigned") && result.at("assigned") == "1";
  String kbName = result.count("kb_name") ? result.at("kb_name") : "";

  if (!ok) {
    oledShowConnectionFailed();
    return;
  }

  if (assigned && kbName.length() > 0) {
    oledShowKbSelected(kbName);
    Monitor.print("Knowledge Base selected: ");
    Monitor.println(kbName);
    return;
  }

  oledShowUnassigned();
  Monitor.println("Card scanned but not assigned.");
}

// ---------------------------------------------------------------------------
// Setup / loop
// ---------------------------------------------------------------------------

void setup() {
  Monitor.begin(9600);
  Bridge.begin();

  Wire.begin();
  if (!display.begin(OLED_I2C_ADDRESS, true)) {
    Monitor.println("OLED initialization failed!");
  } else {
    oledShowStarting();
    delay(1500);
  }

  SPI.begin();
  rfid.PCD_Init();
  delay(100);

  Monitor.println("Smart Learn 2.0 RFID ready.");
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
    delay(1500);
    oledShowReady();
    return;
  }

  oledShowScanning(displayUid);
  Monitor.println("Calling API...");

  std::map<String, String> result;
  bool bridgeOk = Bridge.call("rfid_detected", uid).result(result);

  if (!bridgeOk) {
    Monitor.println("Bridge call failed.");
    oledShowConnectionFailed();
  } else {
    handleScanResult(result);
  }

  rememberScan(uid);
  haltCard();

  delay(2500);
  oledShowReady();
  delay(500);
}
