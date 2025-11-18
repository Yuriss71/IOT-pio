#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN D8
#define RST_PIN D2

MFRC522 rfid(SS_PIN, RST_PIN);

void setup() {
  Serial.begin(115200);
  SPI.begin();
  rfid.PCD_Init();
}

void loop() {
  Serial.println(rfid.PICC_IsNewCardPresent());
  Serial.println(rfid.PICC_ReadCardSerial());
  
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    Serial.println("Aucun badge détecté.");
    delay(500);
    return;
  }

  Serial.print("UID du badge : ");
  for (byte i = 0; i < rfid.uid.size; i++) {
    Serial.print(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
    Serial.print(rfid.uid.uidByte[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  delay(500);
}
