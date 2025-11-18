#include <Arduino.h>

#ifndef RFID_ENABLED
#define RFID_ENABLED 1
#endif

#define TRIG_PIN D1
#define ECHO_PIN D2
#define SOUND_VELOCITY 0.034

unsigned long lastSoundRead = 0;
const unsigned long soundInterval = 100;
bool soundEnabled = true;

long duration;
float distanceCm;

#if RFID_ENABLED
  #include <SPI.h>
  #include <MFRC522.h>
  #define SS_PIN D8
  #define RST_PIN D4

  MFRC522 rfid(SS_PIN, RST_PIN);
  unsigned long lastRFIDRead = 0;
  const unsigned long rfidInterval = 300;
#endif


void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  Serial.println("Capteur à ultrasons initialisé.");

#if RFID_ENABLED
  SPI.begin();
  rfid.PCD_Init();
  delay(4);
  Serial.println("RFID activé");
#else
  Serial.println("RFID désactivé");
#endif
}


void readUltrasonic() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  duration = pulseIn(ECHO_PIN, HIGH, 25000);
  if (duration == 0) return;

  distanceCm = duration * SOUND_VELOCITY / 2;

  Serial.print("[ULTRASON] Distance: ");
  Serial.print(distanceCm);
  Serial.println(" cm");
}


#if RFID_ENABLED
void readRFID() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial())
    return;

  Serial.print("[RFID] UID : ");
  for (byte i = 0; i < rfid.uid.size; i++)
    Serial.printf("%02X ", rfid.uid.uidByte[i]);
  Serial.println();

  soundEnabled = !soundEnabled;
  Serial.print("[RFID] Son ");
  Serial.println(soundEnabled ? "activé" : "désactivé");

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}
#else
void readRFID() {}
#endif


void loop() {
  unsigned long now = millis();

  if (soundEnabled) {
    if (now - lastSoundRead >= soundInterval) {
      lastSoundRead = now;
      readUltrasonic();
    }
  }


#if RFID_ENABLED
  if (now - lastRFIDRead >= rfidInterval) {
    lastRFIDRead = now;
    readRFID();
  }
#endif
}
