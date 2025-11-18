#include <Arduino.h>
#include <WiFiManager.h>
#include <PubSubClient.h>

#ifndef RFID_ENABLED
#define RFID_ENABLED 1
#endif

#if RFID_ENABLED
  #include <SPI.h>
  #include <MFRC522.h>
  #define SS_PIN D8
  #define RST_PIN D4
  MFRC522 rfid(SS_PIN, RST_PIN);
  unsigned long lastRFIDRead = 0;
  const unsigned long rfidInterval = 300;
#endif

#define TRIG_PIN D1
#define ECHO_PIN D2
#define SOUND_VELOCITY 0.034

String deviceID;
WiFiClient espClient;
PubSubClient client(espClient);
const char* mqtt_server = "broker.emqx.io";

unsigned long lastSoundRead = 0;
const unsigned long soundInterval = 400;
bool soundEnabled = true;

const unsigned long maxDistanceCm = 100;
long duration;
float distanceCm;
bool objectDetected = false;
int objectIteration = 0;

unsigned long lastReconnectAttempt = 0;
const unsigned long reconnectInterval = 5000;

String generateDeviceID() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char buf[13];
  sprintf(buf, "%02X%02X%02X%02X", mac[2], mac[3], mac[4], mac[5]);
  return String(buf);
}

void reconnect() {
  if (client.connected()) {
    return;
  }

  unsigned long now = millis();
  if (now - lastReconnectAttempt > reconnectInterval) {
    lastReconnectAttempt = now;
    
    Serial.print("[MQTT] Tentative de connexion (" + deviceID + ")... ");

    if (client.connect(deviceID.c_str())) {
      Serial.println("MQTT connecté");
    } else {
      Serial.print("ECHEC. rc=");
      Serial.print(client.state());
      Serial.println(" (Nouvelle tentative dans 5s)");
    }
  }
}

void setup() {
  Serial.begin(115200);

  deviceID = generateDeviceID();
  Serial.println("Device ID: " + deviceID);

  WiFiManager wm;

  String deviceIDHTML = "<p><b>Ajoutez cet identifiant unique à votre compte:</b> " + deviceID + "</p>";
  WiFiManagerParameter customID(deviceIDHTML.c_str());
  wm.addParameter(&customID);

  String apName = "LIDL_" + deviceID;
  if (!wm.autoConnect(apName.c_str())) {
    ESP.restart();
  }
  Serial.println("Connected! IP: " + WiFi.localIP().toString());

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  Serial.println("Capteur à ultrasons initialisé.");

#if RFID_ENABLED
  SPI.begin();
  rfid.PCD_Init();
  delay(4);
  Serial.println("RFID activé");
#endif

  client.setServer(mqtt_server, 1883);
}

void sendMqtt(String topic, String payload) {
  if (client.connected()) {
    client.publish(topic.c_str(), payload.c_str());
    Serial.println("📤 MQTT envoyé: " + topic + " => " + payload);
  } else {
    Serial.println("⚠️ Impossible d'envoyer MQTT (Déconnecté)");
  }
}

void readUltrasonic() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  duration = pulseIn(ECHO_PIN, HIGH, 25000);

  if (duration == 0) {
    distanceCm = -1;
  } else {
    distanceCm = duration * SOUND_VELOCITY / 2;
  }

  if (distanceCm < maxDistanceCm && distanceCm != -1 && !objectDetected) {
    Serial.print("[ULTRASON] Objet détecté à ");
    Serial.print(distanceCm);
    Serial.println(" cm");
    objectIteration = 0;
    objectDetected = true;
  } else if (objectDetected && (distanceCm >= maxDistanceCm || distanceCm == -1)) {
    objectIteration++;
    if (objectIteration >= 2) {
        objectDetected = false;

        String topic = "ynov/bdx/lidl/" + deviceID + "/count";
        String payload = "{\"change\": 1}";
        sendMqtt(topic, payload);
    }
  }
}

#if RFID_ENABLED
void readRFID() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial())
    return;

    String uuid = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
      if (rfid.uid.uidByte[i] < 0x10) {
        uuid += "0";
      }
      uuid += String(rfid.uid.uidByte[i], HEX);
    }
    uuid.toUpperCase();

  String topic = "ynov/bdx/lidl/" + deviceID + "/toggle";
  String payload = "{";
  payload += "\"uuid\": \"" + uuid + "\"";
  payload += "}";
  sendMqtt(topic, payload);

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}
#endif

void loop() {
  unsigned long now = millis();

  reconnect();
  client.loop();

  if (now - lastSoundRead >= soundInterval) {
    lastSoundRead = now;
    readUltrasonic();
  }

#if RFID_ENABLED
  if (now - lastRFIDRead >= rfidInterval) {
    lastRFIDRead = now;
    readRFID();
  }
#endif
}