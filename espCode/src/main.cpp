

#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>

#define SS_PIN 5
#define RST_PIN 22



MFRC522 rfid(SS_PIN, RST_PIN);

String user_id_string = "";
int user_id_int;
String statusStr = "true";
bool statusSwap = false;
const int buzzerPin = 25;


/**************************** 
const char* ssid = "COY-85996";
const char* password = "8f7z-czyd-re7j-lp0t";
const char* serverUrl = "http://192.168.1.105:18080/api/rfid/log";
/* ========================== */

void setup() {
  Serial.begin(115200);
  SPI.begin();
  rfid.PCD_Init();
 

  Serial.println("RFID Leser bereit");

 /***************************
  WiFi.begin(ssid, password);
  Serial.print("Verbinde mit WLAN");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWLAN verbunden");
  /* ========================== */
}

void loop() {
  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }

  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }

requestAPI(converIdToInt(readDataFromRFID()));
delay(1000);



}


void buzzKommen(int buzzTime){
  tone(buzzerPin, 1000);  // 1 kHz
  delay(buzzTime);
  noTone(buzzerPin);      // aus
  delay(buzzTime);
  tone(buzzerPin, 1000);  // 1 kHz
  delay(buzzTime);
  noTone(buzzerPin);      // aus
}


void buzzGehen(int buzzTime){
  tone(buzzerPin, 1000);  // 1 kHz
  delay(buzzTime);
  noTone(buzzerPin);      // aus
}


void buzzUnbekannt(int buzzTime){
  tone(buzzerPin, 500);  // 0.5 kHz
  delay(buzzTime);
  tone(buzzerPin, 1000);  // 1 kHz
  delay(buzzTime);
  tone(buzzerPin, 750);  // 0.75 kHz
  delay(buzzTime);
  noTone(buzzerPin);      // aus
}





String readDataFromRFID(){
  user_id_string = "";

  for (int i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) {
      user_id_string += "0";
    }
    user_id_string += String(rfid.uid.uidByte[i], HEX);
  }

  user_id_string.toUpperCase();

  Serial.print("Karten UID: ");
  Serial.println(user_id_string);

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  return user_id_string;
}

int converIdToInt(String string_id){
  if(string_id == "BB29A032") {
    return 101;
  }
  else if(string_id == "FB129E32") {
    return 102;
  }
  else if(string_id == "9B519132") {
    return 103;
  }
  else{
    return 0;
  }
}

void requestAPI(int variabel){
  if(variabel == 0){
    Serial.println("Karte Unbekannt!");
    Serial.println(variabel);
    buzzUnbekannt(50);    // delay Time 50ms
    return;
  }

  Serial.print("Karten API_UID: ");
  Serial.println(variabel);

//**********************************//

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WLAN nicht verbunden!");
    return;
  }

  HTTPClient http;
  http.begin(serverUrl);
  http.addHeader("Content-Type", "application/json");
  if(statusSwap){
    statusStr = "false";
  }
  else{
    statusStr = "true";
  }

  String json =
  "{"
    "\"user_id\":" + String(variabel) + ","
    "\"status\":" + statusStr +
  "}";

  int httpResponseCode = http.PUT(json);

  Serial.print("HTTP Status: ");
  Serial.println(httpResponseCode);

  if (httpResponseCode >= 200 && httpResponseCode <300) {
    Serial.println(http.getString());

  if(!statusSwap){
    buzzKommen(50);  // Delay Time 50ms
  }
  else{
    buzzGehen(750);  // Delay Time 750ms
    }
  }
  
 
   statusSwap = !statusSwap;
  

  //Serial.println(statusSwap);

  http.end();
  /* ========================== */
}