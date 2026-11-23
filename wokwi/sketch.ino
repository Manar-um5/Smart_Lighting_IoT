
#include <WiFi.h>
#include "ThingSpeak.h"

#define LDR_PIN 36
#define PIR_PIN 15
#define LED_PIN 12

// Parametres LED
int seuilNuit = 2000;
int maxPWM = 255;
int minPWM = 50;

// Configuration WiFi
const char* WIFI_NAME = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";

// Configuration ThingSpeak
const unsigned long myChannelNumber = 3592262;
const char* myApiKey = "YOUR_THINGSPEAK_WRITE_API_KEY";

WiFiClient client;

unsigned long dernierEnvoi = 0;
const unsigned long intervalleEnvoi = 20000;

// Memorisation du mouvement
bool mouvementDetecte = false;

void setup() {
  Serial.begin(115200);

  pinMode(LDR_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  analogWrite(LED_PIN, 0);

  Serial.println("============================");
  Serial.println("SMART CITY - ECLAIRAGE IoT");
  Serial.println("ESP32 + ThingSpeak");
  Serial.println("============================");

  WiFi.begin(WIFI_NAME, WIFI_PASSWORD);
  Serial.print("Connexion WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println(" Connecte !");
  Serial.println(WiFi.localIP());

  ThingSpeak.begin(client);
}

void loop() {

  // Lecture des capteurs
  int luminosite = analogRead(LDR_PIN);
  int mouvement = digitalRead(PIR_PIN);

  // Memoriser tout mouvement detecte
  if (mouvement == HIGH) {
    mouvementDetecte = true;
  }

  int ledPWM = 0;
  int consoEnergetique = 0;

  // Gestion intelligente de la LED
  if (luminosite > seuilNuit) {

    if (mouvement == HIGH) {

      ledPWM = map(
        luminosite,
        seuilNuit,
        4095,
        minPWM,
        maxPWM
      );

      Serial.print("Mode : NUIT + MOUVEMENT");

    } else {

      ledPWM = map(
        luminosite,
        seuilNuit,
        4095,
        minPWM / 2,
        maxPWM / 2
      );

      Serial.print("Mode : NUIT SANS MOUVEMENT");
    }

    ledPWM = constrain(ledPWM, 0, maxPWM);

  } else {

    ledPWM = 0;
    Serial.print("Mode : JOUR");
  }

  // Application PWM
  analogWrite(LED_PIN, ledPWM);

  // Estimation de consommation
  consoEnergetique = map(ledPWM, 0, maxPWM, 0, 100);

  // Affichage console
  Serial.print(" | Luminosite : ");
  Serial.print(luminosite);

  Serial.print(" | PIR : ");
  Serial.print(mouvement);

  Serial.print(" | LED PWM : ");
  Serial.print(ledPWM);

  Serial.print(" | Conso estimee : ");
  Serial.print(consoEnergetique);
  Serial.println("%");

  // Envoi ThingSpeak toutes les 20 secondes
  if (millis() - dernierEnvoi >= intervalleEnvoi) {

    dernierEnvoi = millis();

    if (WiFi.status() == WL_CONNECTED) {

      ThingSpeak.setField(1, luminosite);
      ThingSpeak.setField(2, ledPWM);
      ThingSpeak.setField(3, mouvement);
      ThingSpeak.setField(4, consoEnergetique);

      int x = ThingSpeak.writeFields(
        myChannelNumber,
        myApiKey
      );

      if (x == 200) {
        Serial.println("ThingSpeak : Envoi reussi !");
        mouvementDetecte = false;
      } else {
        Serial.print("Erreur ThingSpeak : ");
        Serial.println(x);
      }

    } else {
      Serial.println("WiFi deconnecte");
      WiFi.reconnect();
    }
  }

  delay(2000);
}
