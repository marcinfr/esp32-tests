#include <Wire.h>
#include <Adafruit_PN532.h>
#include <SPI.h>
#include <SD.h>
#include "Audio.h"

#define SD_CS 5

#define I2S_DOUT 22
#define I2S_BCLK 26
#define I2S_LRC 25

#define SDA_PIN 32
#define SCL_PIN 33

#define PN532_IRQ   27
#define PN532_RESET 4

#define LED_PIN 13
#define LED_PIN_2 21
#define BLUE_LED 2

#define VOLUME_PIN 34

Audio audio;
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET, &Wire);

bool cardPresent = false;
bool audioIsRunning = false;
bool systemAudioIsRunning = false;
unsigned long lastRFIDCheck = 0; // RFID sprawdzamy co RFID_INTERVAL ms. // Audio działa pomiędzy sprawdzeniami bez przerw. 
const unsigned long RFID_INTERVAL = 1000;

// Klucz NDEF dla MIFARE Classic
uint8_t NDEF_KEY[6] = {
  0xD3, 0xF7, 0xD3,
  0xF7, 0xD3, 0xF7
};

#define BUFFER_SIZE 192

// ESP32 będzie budził się co 2000 ms, 
// sprawdzał PN532 i jeżeli nie ma karty, 
// ponownie zasypiał. 
#define SLEEP_TIME_US 2000000ULL

uint8_t buffer[BUFFER_SIZE];
uint16_t bufferLength;



// =====================================================
// Odczyt NDEF z MIFARE Classic
// =====================================================

bool readNDEF(uint8_t *uid, uint8_t uidLength)
{
  bufferLength = 0;

  uint8_t data[16];

  // Sektory 1-4 wystarczają dla typowego krótkiego NDEF
  for (uint8_t sector = 1; sector <= 4; sector++)
  {
    uint8_t firstBlock = sector * 4;

    if (!nfc.mifareclassic_AuthenticateBlock(
          uid,
          uidLength,
          firstBlock,
          0,
          NDEF_KEY))
    {
      continue;
    }

    // 3 bloki danych, pomijamy Sector Trailer
    for (uint8_t block = firstBlock;
         block < firstBlock + 3;
         block++)
    {
      if (bufferLength + 16 > BUFFER_SIZE)
        return false;

      if (nfc.mifareclassic_ReadDataBlock(block, data))
      {
        memcpy(&buffer[bufferLength], data, 16);
        bufferLength += 16;
      }
    }
  }

  return bufferLength > 0;
}

// =====================================================
// Odczyt tekstu NDEF
// =====================================================

String getNDEFText()
{
  int ndefStart = -1;

  // Szukamy TLV 03
  for (uint16_t i = 0; i < bufferLength; i++)
  {
    if (buffer[i] == 0x03)
    {
      ndefStart = i;
      break;
    }
  }

  if (ndefStart < 0)
    return "";

  uint16_t pos = ndefStart + 1;

  if (pos >= bufferLength)
    return "";

  uint16_t messageLength = buffer[pos++];

  if (messageLength == 0 || pos >= bufferLength)
    return "";

  uint8_t header = buffer[pos++];

  bool shortRecord = header & 0x10;
  uint8_t tnf = header & 0x07;

  if (pos >= bufferLength)
    return "";

  uint8_t typeLength = buffer[pos++];

  uint32_t payloadLength;

  if (shortRecord)
  {
    if (pos >= bufferLength)
      return "";

    payloadLength = buffer[pos++];
  }
  else
  {
    if (pos + 4 > bufferLength)
      return "";

    payloadLength =
      ((uint32_t)buffer[pos] << 24) |
      ((uint32_t)buffer[pos + 1] << 16) |
      ((uint32_t)buffer[pos + 2] << 8) |
      buffer[pos + 3];

    pos += 4;
  }

  uint8_t idLength = 0;

  if (header & 0x08)
  {
    if (pos >= bufferLength)
      return "";

    idLength = buffer[pos++];
  }

  // Musi być rekord typu Text
  if (tnf != 0x01 || typeLength != 1 || buffer[pos] != 'T')
    return "";

  pos += typeLength;

  // ID rekordu
  pos += idLength;

  if (pos >= bufferLength || payloadLength < 1)
    return "";

  uint8_t status = buffer[pos++];

  uint8_t languageLength = status & 0x3F;

  if (languageLength + 1 > payloadLength)
    return "";

  pos += languageLength;

  uint32_t textLength =
    payloadLength - 1 - languageLength;

  Serial.print("Tekst: ");
  String text = "";

  for (uint32_t i = 0; i < textLength; i++)
  {
    text += (char)buffer[pos + i];
  }

  Serial.print("Tekst RFID: ");
  Serial.println(text);

  return text;
}

void playSystemAudio(String message, String lang = "pl")
{
  playAudio("/system/" + message + "_" + lang + ".mp3");
  systemAudioIsRunning = true;
}


// =====================================================
// SETUP
// =====================================================

void setup()
{
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  pinMode(LED_PIN_2, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(LED_PIN_2, LOW);

  Wire.begin(SDA_PIN, SCL_PIN);

  nfc.begin();
  nfc.getFirmwareVersion();
  nfc.SAMConfig();

  if (!SD.begin(SD_CS)) {
    Serial.println("SD NIE wykryta");
    while (1);
  }

  Serial.println("SD OK");
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  setVolume();

  esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
  if ((int)cause == 0) { 
    playSystemAudio("welcome");
  } else {
    Serial.println("wakeup by card");
    // wakeup by card
    //cardPresent= true;
    //systemAudioIsRunning = true;
  }
}

void goToSleep(bool deep = true) {
  if (deep && !cardPresent) {
    Serial.println("DEEP SLEEP");
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    esp_sleep_enable_ext0_wakeup((gpio_num_t)PN532_IRQ, 0);
    esp_deep_sleep_start();
  } else {
    Serial.println("LIGHT SLEEP"); 
    // Wszystkie LED wyłączone przed snem 
    digitalWrite(LED_PIN, LOW); 
    digitalWrite(LED_PIN_2, LOW); 
    digitalWrite(BLUE_LED, LOW);
    // Wybudzenie po SLEEP_TIME_US ms 
    esp_sleep_enable_timer_wakeup(SLEEP_TIME_US); 
    // Wejście w Light Sleep. 
    esp_light_sleep_start(); 
  }
}

void playAudio(String filename)
{
  Serial.print("Szukam pliku: ");
  Serial.println(filename);

  if (SD.exists(filename))
  {
    Serial.println("Plik znaleziony!");

    // ----------------------------------------------
    // START MP3
    // ----------------------------------------------
    audio.connecttoFS(SD, filename.c_str());
    systemAudioIsRunning = false;
    audioIsRunning = true;
  }
  else
  {
    Serial.println("Pliku NIE ma na karcie SD");
  }
}

void stopAudio()
{
    if (audioIsRunning || systemAudioIsRunning) {
      audioIsRunning = false;
      systemAudioIsRunning = false;
      audio.stopSong();
      digitalWrite(LED_PIN_2, LOW);
      digitalWrite(LED_PIN, LOW);
      Serial.println("MP3 zakonczone");
    }
}

void setVolume()
{
  int potValue = analogRead(VOLUME_PIN);
  int volume = map(potValue, 0, 4095, 0, 21);
  audio.setVolume(volume);
}

void playLedShow()
{
    if (audio.getBassLevel() > 5000) {
      digitalWrite(LED_PIN, HIGH);
    } else {
      digitalWrite(LED_PIN, LOW);
    }

    if (audio.getBassLevel() < 1000) {
      digitalWrite(LED_PIN_2, HIGH);
    } else {
      digitalWrite(LED_PIN_2, LOW);
    }
}

void processRFID() {

  if (audioIsRunning && millis() - lastRFIDCheck < RFID_INTERVAL)
  {
    return;
  }
  lastRFIDCheck = millis();

  uint8_t uid[7];
  uint8_t uidLength;

  bool success = nfc.readPassiveTargetID(
    PN532_MIFARE_ISO14443A,
    uid,
    &uidLength,
    20
  );

  // KARTA WYKRYTA
  if (success) {
    if (!cardPresent) {
      cardPresent = true;

      Serial.println("KARTA PRZYLOZONA");
      digitalWrite(LED_PIN_2, HIGH);

      // Odczytaj NDEF
      if (readNDEF(uid, uidLength)) {
        String text = getNDEFText();
        if (text.length() > 0) {

          String filename = "/" + text + ".mp3";
          playAudio(filename);
        }
      }
    }
  } else {
    // KARTY NIE MA
    if (cardPresent) {
      cardPresent = false;
      Serial.println("KARTA ODDALONA");
      stopAudio();
    }
  }
}

// =====================================================
// LOOP
// =====================================================
void loop()
{
  digitalWrite(BLUE_LED, HIGH);

  setVolume();
  // To musi być wykonywane bardzo często!
  audio.loop();
  playLedShow();

  processRFID();

  if (!audio.isRunning() && audioIsRunning) {
    stopAudio();
  }

  if ((!cardPresent || !audioIsRunning) && !systemAudioIsRunning) { 
    goToSleep(); 
  }
}
