#include <SPI.h>
#include <SD.h>
#include "Audio.h"

#define SD_CS 5

#define I2S_DOUT 22
#define I2S_BCLK 26
#define I2S_LRC 25

Audio audio;

void setup() {

  Serial.begin(115200);

  Serial.println("Start");

  SPI.begin(18, 19, 23, SD_CS);

  if (!SD.begin(SD_CS)) {
    Serial.println("SD NIE wykryta");
    while (1);
  }

  Serial.println("SD OK");

  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);

  audio.setVolume(15);

  audio.connecttoFS(SD, "/song01.mp3");

  Serial.println("Start MP3");
}

void loop() {
  audio.loop();
}
