#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <MFRC522.h>

// -----------------------
// RFID (RC522) SETTINGS
// -----------------------
#define SS_PIN  5
#define RST_PIN 4
MFRC522 rfid(SS_PIN, RST_PIN);

// Cooldown so a card held on the reader does NOT retrigger
String lastUID = "";
unsigned long lastScanTime = 0;
const unsigned long scanCooldown = 2000;  // 2 seconds debounce

// -----------------------
// WIFI SETTINGS
// -----------------------
const char* ssid     = "BBZ-WLAN";
const char* password = "Das2.letztePW";

// -----------------------
// SERVER SETTINGS
// -----------------------
const char* serverUrl = "http://192.168.190.90:18080/api/rfid/log";
const int deviceId = 1234;  // Identify this device to the server

// -----------------------
// SEND RFID TAG DATA (QUICK & DIRTY)
// -----------------------
void sendTagToServer(String uid) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected, retrying...");
        WiFi.reconnect();
        return;
    }

    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");

    StaticJsonDocument<256> doc;

    // QUICK & DIRTY FIX: match old server fields
    doc["device_id"] = deviceId;
    doc["name"]      = uid;     // Tag UID shoved into "name"
    doc["status"]    = true;    // Server requires a boolean

    String jsonStr;
    serializeJson(doc, jsonStr);

    Serial.print("Sending JSON: ");
    Serial.println(jsonStr);

    int httpCode = http.PUT(jsonStr);

    if (httpCode > 0) {
        Serial.print("HTTP Response: ");
        Serial.println(httpCode);

        String payload = http.getString();
        if (payload.length() > 0) {
            Serial.println("Server reply:");
            Serial.println(payload);
        }
    } else {
        Serial.printf("HTTP Error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
}

// -----------------------
// SETUP
// -----------------------
void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.print("Connecting to WiFi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected!");

    SPI.begin();
    rfid.PCD_Init();
    Serial.println("RC522 initialized!");
}

// -----------------------
// LOOP
// -----------------------
void loop() {

    // No new card
    if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
        return;
    }

    // Convert UID to hex string
    String tagUID = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
        char hexStr[4];
        sprintf(hexStr, "%02X", rfid.uid.uidByte[i]);
        tagUID += hexStr;
    }

    // ---------- DEBOUNCE HERE ----------
    unsigned long now = millis();

    if (tagUID == lastUID && (now - lastScanTime) < scanCooldown) {
        // Ignore repeated scans of same card within cooldown
        Serial.println("Duplicate scan ignored...");
        rfid.PICC_HaltA();
        rfid.PCD_StopCrypto1();
        return;
    }

    // Accept the scan
    lastUID = tagUID;
    lastScanTime = now;

    Serial.print("TAG DETECTED: ");
    Serial.println(tagUID);

    sendTagToServer(tagUID);

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
}
