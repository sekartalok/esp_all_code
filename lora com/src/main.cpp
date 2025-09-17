#include <RadioLib.h>

// LoRa Module pins
#define NSS   7
#define RST   8
#define DIO1  33
#define BUSY  34
#define SCK   5
#define MISO  3
#define MOSI  6

SPIClass spiLoRa(FSPI);
SX1262 radio = new Module(NSS, DIO1, RST, BUSY, spiLoRa);

// --- Brute-force Parameters ---
// Frequency range (MHz)
const float freqMin = 150.0;
const float freqMax = 960.0;
const float freqStep = 0.2; // 200 kHz step
// Sync Word range
const uint8_t syncWordMin = 0x00;
const uint8_t syncWordMax = 0xFF;
// Spreading Factor range
const uint8_t sfMin = 7;
const uint8_t sfMax = 12;
// Bandwidths to test (kHz)
const float bandwidths[] = {7.8, 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125.0, 250.0, 500.0};
const uint8_t bandwidthCount = sizeof(bandwidths) / sizeof(bandwidths[0]);
// Coding Rates to test (4/5, 4/6, 4/7, 4/8)
const uint8_t codingRates[] = {5, 6, 7, 8};
const uint8_t codingRateCount = sizeof(codingRates) / sizeof(codingRates[0]);
// Output Power range (dBm)
const uint8_t outputPowerMin = 2;
const uint8_t outputPowerMax = 22;
const uint8_t outputPowerStep = 2;
// CRC On/Off
const bool crcEnabled[] = {true, false};
const uint8_t crcCount = sizeof(crcEnabled) / sizeof(crcEnabled[0]);
// Header Type (Explicit/Implicit)
const bool explicitHeader[] = {true, false};
const uint8_t headerCount = sizeof(explicitHeader) / sizeof(explicitHeader[0]);
// Time to listen on each combination (milliseconds)
const unsigned long interval = 10000;
// When testing implicit header mode, a packet length must be specified.
const uint8_t FIXED_PACKET_LENGTH = 240;  // Match the receive packet length used in working code

// --- State Variables ---
unsigned long prevMillis = 0;
bool messageReceived = false;

// Current parameters being tested
float currentFreq = freqMin;           // Start frequency at minimum (150 MHz)
uint16_t currentSyncWord = syncWordMin; // Sync word minimum (0x00)
uint8_t currentSF = sfMin;             // SF minimum (7)
uint8_t currentBWIndex = 0;            // bandwidths[0] = 7.8 kHz (lowest bandwidth)
uint8_t currentCRIndex = 0;            // codingRates[0] = 5 (4/5)
uint8_t currentOutputPower = outputPowerMin;
uint8_t currentCRCIndex = 0;           // CRC enabled first
uint8_t currentHeaderIndex = 0;        // Explicit header first


// Forward declarations
void applyRadioSettings();
void nextCombination();

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("Starting RadioLib SX1262 Brute-force Scanner...");
  spiLoRa.begin(SCK, MISO, MOSI);

  int state = radio.begin(currentFreq);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("Radio init failed with code ");
    Serial.println(state);
    while (true);
  }

  applyRadioSettings();
  // Start blocking reception as in your working code, to test signal detection
  radio.startReceive();
  prevMillis = millis();
}

void applyRadioSettings() {
  radio.setSyncWord((uint8_t)currentSyncWord);
  radio.setSpreadingFactor(currentSF);
  radio.setBandwidth(bandwidths[currentBWIndex]);
  radio.setCodingRate(codingRates[currentCRIndex]);
  radio.setOutputPower(currentOutputPower);
  radio.setCRC(crcEnabled[currentCRCIndex]);

  if (explicitHeader[currentHeaderIndex]) {
    radio.explicitHeader();
  } else {
    radio.implicitHeader(FIXED_PACKET_LENGTH);
  }

  Serial.print("Trying Combo -> Freq: ");
  Serial.print(currentFreq, 3);
  Serial.print(" MHz, SyncWord: 0x");
  if (currentSyncWord < 0x10) Serial.print("0");
  Serial.print(currentSyncWord, HEX);
  Serial.print(", SF: ");
  Serial.print(currentSF);
  Serial.print(", BW: ");
  Serial.print(bandwidths[currentBWIndex]);
  Serial.print(" kHz, CR: 4/");
  Serial.print(codingRates[currentCRIndex]);
  Serial.print(", CRC: ");
  Serial.print(crcEnabled[currentCRCIndex] ? "On" : "Off");
  Serial.print(", Header: ");
  Serial.println(explicitHeader[currentHeaderIndex] ? "Explicit" : "Implicit");
}

void nextCombination() {
  currentHeaderIndex++;
  if (currentHeaderIndex >= headerCount) {
    currentHeaderIndex = 0;
    currentCRCIndex++;
    if (currentCRCIndex >= crcCount) {
      currentCRCIndex = 0;
      currentOutputPower += outputPowerStep;
      if (currentOutputPower > outputPowerMax) {
        currentOutputPower = outputPowerMin;
        currentCRIndex++;
        if (currentCRIndex >= codingRateCount) {
          currentCRIndex = 0;
          currentBWIndex++;
          if (currentBWIndex >= bandwidthCount) {
            currentBWIndex = 0;
            currentSF++;
            if (currentSF > sfMax) {
              currentSF = sfMin;
              currentSyncWord++;
              if (currentSyncWord > syncWordMax) {
                currentSyncWord = syncWordMin;
                currentFreq += freqStep;
                if (currentFreq > freqMax) {
                  currentFreq = freqMin;
                  Serial.println("All combinations tried. Restarting scan.");
                }
              }
            }
          }
        }
      }
    }
  }

  int state = radio.begin(currentFreq);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("Radio re-init failed with code ");
    Serial.println(state);
  }

  applyRadioSettings();
  radio.startReceive();
  prevMillis = millis();
}

void loop() {
  if (messageReceived) {
    return;
  }

  String str;
  int state = radio.receive(str, FIXED_PACKET_LENGTH);   // Blocking receive with fixed packet length

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("\n-----------------------------------------");
    Serial.println(">>> MESSAGE FOUND! <<<");
    Serial.println("Matching configuration:");
    applyRadioSettings();
    Serial.print("Message RSSI: ");
    Serial.print(radio.getRSSI());
    Serial.println(" dBm");
    Serial.print("Message SNR: ");
    Serial.print(radio.getSNR());
    Serial.println(" dB");
    Serial.print("Message Content: ");
    Serial.println(str);
    Serial.println("-----------------------------------------\n");
    messageReceived = true;
    radio.standby();
    return;
  }
  else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
    // Timeout, no data received, try next combination if interval passed
    if (millis() - prevMillis >= interval) {
      radio.standby();
      Serial.println("Timeout, trying next combination...");
      nextCombination();
    }
  }
  else {
    Serial.print("Receive failed with error code ");
    Serial.println(state);
    if (millis() - prevMillis >= interval) {
      radio.standby();
      Serial.println("Error or no packet, trying next combination...");
      nextCombination();
    }
  }
}
