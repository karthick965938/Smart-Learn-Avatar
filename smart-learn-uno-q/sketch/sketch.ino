#include <Arduino_RouterBridge.h>
#include <SPI.h>
#include <MFRC522.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

// =====================================================
// RFID
// =====================================================

#define SS_PIN 10
#define RST_PIN 9

MFRC522 rfid(SS_PIN, RST_PIN);


// =====================================================
// OLED
// =====================================================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_I2C_ADDRESS 0x3C

Adafruit_SH1106G display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  -1
);


// =====================================================
// RFID state
// =====================================================

String lastUID = "";

unsigned long lastCallTime = 0;

const unsigned long CALL_COOLDOWN = 3000;


// =====================================================
// OLED functions
// =====================================================

void oledHeader(String title) {

  display.clearDisplay();

  display.setTextColor(SH110X_WHITE);

  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println("UNO Q RFID");

  display.drawLine(
    0, 10,
    127, 10,
    SH110X_WHITE
  );

  display.setCursor(0, 14);
  display.println(title);
}


void oledWaiting() {

  display.clearDisplay();

  display.setTextColor(SH110X_WHITE);

  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println("UNO Q RFID");

  display.drawLine(
    0, 10,
    127, 10,
    SH110X_WHITE
  );

  display.setTextSize(1);

  display.setCursor(0, 25);
  display.println("RFID READER READY");

  display.setCursor(0, 43);
  display.println("Tap your card...");

  display.display();
}


void oledChecking(String displayUID) {

  oledHeader("CARD DETECTED");

  display.setCursor(0, 27);
  display.println("UID:");

  display.setCursor(0, 38);
  display.println(displayUID);

  display.setCursor(0, 52);
  display.println("Checking...");

  display.display();
}


void oledSuccess(String displayUID) {

  oledHeader("REQUEST SENT");

  display.setCursor(0, 25);
  display.println("UID:");

  display.setCursor(0, 36);
  display.println(displayUID);

  display.setCursor(0, 51);
  display.println("API OK");

  display.display();
}


void oledFailed(String displayUID) {

  oledHeader("API RESPONSE");

  display.setCursor(0, 25);
  display.println("UID:");

  display.setCursor(0, 36);
  display.println(displayUID);

  display.setCursor(0, 51);
  display.println("CHECK SERVER");

  display.display();
}


void oledSkipped(String displayUID) {

  oledHeader("ALREADY SCANNED");

  display.setCursor(0, 27);
  display.println("UID:");

  display.setCursor(0, 38);
  display.println(displayUID);

  display.setCursor(0, 52);
  display.println("Please remove card");

  display.display();
}


// =====================================================
// Setup
// =====================================================

void setup() {

  Monitor.begin(9600);

  Bridge.begin();

  // ---------------------------------------------------
  // OLED
  // ---------------------------------------------------

  Wire.begin();

  if (!display.begin(OLED_I2C_ADDRESS, true)) {

    Monitor.println("OLED initialization failed!");

  } else {

    display.clearDisplay();

    display.setTextColor(SH110X_WHITE);

    display.setTextSize(1);

    display.setCursor(0, 10);
    display.println("UNO Q RFID SYSTEM");

    display.setCursor(0, 30);
    display.println("Starting...");

    display.display();

    delay(1500);
  }


  // ---------------------------------------------------
  // RFID
  // ---------------------------------------------------

  SPI.begin();

  rfid.PCD_Init();

  delay(100);

  Monitor.println("===============================");
  Monitor.println("UNO Q RFID API SYSTEM");
  Monitor.println("===============================");
  Monitor.println("RFID reader ready.");
  Monitor.println("Waiting for card...");


  oledWaiting();
}


// =====================================================
// Main loop
// =====================================================

void loop() {

  // ---------------------------------------------------
  // Look for card
  // ---------------------------------------------------

  if (!rfid.PICC_IsNewCardPresent()) {

    delay(100);

    return;
  }


  // ---------------------------------------------------
  // Read card
  // ---------------------------------------------------

  if (!rfid.PICC_ReadCardSerial()) {

    return;
  }


  // ---------------------------------------------------
  // Build UID
  //
  // API UID:
  // 0A308005
  //
  // OLED UID:
  // 0A:30:80:05
  // ---------------------------------------------------

  String uid = "";
  String displayUID = "";

  for (byte i = 0; i < rfid.uid.size; i++) {

    if (rfid.uid.uidByte[i] < 0x10) {

      uid += "0";
      displayUID += "0";
    }


    String byteString = String(
      rfid.uid.uidByte[i],
      HEX
    );


    uid += byteString;

    displayUID += byteString;


    if (i < rfid.uid.size - 1) {

      displayUID += ":";
    }
  }


  uid.toUpperCase();
  displayUID.toUpperCase();


  // ---------------------------------------------------
  // Serial
  // ---------------------------------------------------

  Monitor.print("RFID detected: ");
  Monitor.println(uid);


  // ---------------------------------------------------
  // Check cooldown
  // ---------------------------------------------------

  unsigned long now = millis();


  if (
    uid == lastUID &&
    (now - lastCallTime) <= CALL_COOLDOWN
  ) {

    Monitor.println(
      "Same card - API call skipped."
    );


    oledSkipped(displayUID);

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();

    delay(1500);

    oledWaiting();

    return;
  }


  // ---------------------------------------------------
  // Card detected
  // ---------------------------------------------------

  oledChecking(displayUID);

  Monitor.println("Calling API...");


  // ---------------------------------------------------
  // Call Python Bridge
  // ---------------------------------------------------

  bool success = Bridge.call(
    "rfid_detected",
    uid
  );


  // ---------------------------------------------------
  // API result
  // ---------------------------------------------------

  if (success) {

    Monitor.println(
      "Bridge call completed."
    );

    oledSuccess(displayUID);

  } else {

    Monitor.println(
      "Bridge call returned false."
    );

    oledFailed(displayUID);
  }


  // ---------------------------------------------------
  // Save scan
  // ---------------------------------------------------

  lastUID = uid;

  lastCallTime = now;


  // ---------------------------------------------------
  // Stop RFID communication
  // ---------------------------------------------------

  rfid.PICC_HaltA();

  rfid.PCD_StopCrypto1();


  delay(2500);


  // ---------------------------------------------------
  // Ready again
  // ---------------------------------------------------

  oledWaiting();

  delay(500);
}
