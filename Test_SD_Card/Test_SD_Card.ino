#include <SPI.h>
#include <SD.h>
#include "Audio.h"

#define CS_PIN 5
#define LED_PIN 13

#define I2S_DOUT 22
#define I2S_BCLK 26
#define I2S_LRC 25

Audio audio;

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

    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume(15);   // 0-21
    audio.connecttoFS(SD, "/song01.mp3");
  }
}

void loop() {
  audio.loop();
}
