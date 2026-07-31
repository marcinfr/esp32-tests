#include <SPI.h>
#include <SD.h>
#include "Audio.h"

#define CS_PIN 5
#define LED_PIN 13

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  Serial.begin(115200);

  Serial.println("Start SD test");

  if (!SD.begin(CS_PIN)) {
    Serial.println("SD NIE wykryta");
    return;
  }

  Serial.println("SD OK!");

  uint8_t cardType = SD.cardType();

  if(cardType == CARD_NONE){
    Serial.println("Brak karty");
  } else {
    Serial.println("Karta obecna");
  }

  Serial.print("Rozmiar MB: ");
  Serial.println(SD.cardSize() / (1024 * 1024));

  File f = SD.open("/test.txt", FILE_WRITE);

  if (f) {
    f.println("ESP32 OK");
    f.close();
    Serial.println("Zapis OK");
    digitalWrite(LED_PIN, HIGH);
  }
}

void loop() {
}
