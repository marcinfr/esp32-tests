#include <Wire.h>
#include <Adafruit_PN532.h>

#define SDA_PIN 32
#define SCL_PIN 33

#define PN532_IRQ   27
#define PN532_RESET 26

Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET, &Wire);

// Klucz NDEF dla MIFARE Classic
uint8_t NDEF_KEY[6] = {
  0xD3, 0xF7, 0xD3,
  0xF7, 0xD3, 0xF7
};

#define BUFFER_SIZE 192

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

bool getNDEFText()
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
    return false;

  uint16_t pos = ndefStart + 1;

  if (pos >= bufferLength)
    return false;

  uint16_t messageLength = buffer[pos++];

  if (messageLength == 0 || pos >= bufferLength)
    return false;

  uint8_t header = buffer[pos++];

  bool shortRecord = header & 0x10;
  uint8_t tnf = header & 0x07;

  if (pos >= bufferLength)
    return false;

  uint8_t typeLength = buffer[pos++];

  uint32_t payloadLength;

  if (shortRecord)
  {
    if (pos >= bufferLength)
      return false;

    payloadLength = buffer[pos++];
  }
  else
  {
    if (pos + 4 > bufferLength)
      return false;

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
      return false;

    idLength = buffer[pos++];
  }

  // Musi być rekord typu Text
  if (tnf != 0x01 || typeLength != 1 || buffer[pos] != 'T')
    return false;

  pos += typeLength;

  // ID rekordu
  pos += idLength;

  if (pos >= bufferLength || payloadLength < 1)
    return false;

  uint8_t status = buffer[pos++];

  uint8_t languageLength = status & 0x3F;

  if (languageLength + 1 > payloadLength)
    return false;

  pos += languageLength;

  uint32_t textLength =
    payloadLength - 1 - languageLength;

  Serial.print("Tekst: ");

  for (uint32_t i = 0; i < textLength; i++)
  {
    Serial.write(buffer[pos + i]);
  }

  Serial.println();

  return true;
}


// =====================================================
// SETUP
// =====================================================

void setup()
{
  Serial.begin(115200);

  Wire.begin(SDA_PIN, SCL_PIN);

  nfc.begin();

  nfc.getFirmwareVersion();

  nfc.SAMConfig();
}


// =====================================================
// LOOP
// =====================================================

void loop()
{
  uint8_t uid[7];
  uint8_t uidLength;

  bool success = nfc.readPassiveTargetID(
    PN532_MIFARE_ISO14443A,
    uid,
    &uidLength,
    1000
  );

  if (!success) {
    Serial.println("Brak NDEF");
    return;
  }

  // UID
  /**
  Serial.print("UID: ");

  for (uint8_t i = 0; i < uidLength; i++)
  {
    if (uid[i] < 0x10)
      Serial.print("0");

    Serial.print(uid[i], HEX);

    if (i < uidLength - 1)
      Serial.print(":");
  }

  Serial.println();
  */

  // NDEF
  if (readNDEF(uid, uidLength))
  {
    getNDEFText();
  }
  else
  {
    Serial.println("Brak NDEF");
  }

  Serial.println();

  // Zapobiega wielokrotnemu odczytowi tej samej karty
  //delay(3000);
}
