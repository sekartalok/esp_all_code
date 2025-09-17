#include <RadioLib.h>
// --- CONNECTIONS - UNCHANGED ---
#define NSS   7
#define RST   8
#define DIO1  33
#define BUSY  34
#define SCK   5
#define MISO  3
#define MOSI  6

SPIClass spiLoRa(FSPI);
SX1262 radio = new Module(NSS, DIO1, RST, BUSY, spiLoRa);

// --- BRUTE-FORCE PARAMETERS ---
const float freqMin = 150.0;
const float freqMax = 960.0;
const float freqStep = 0.2;
const uint16_t syncWordMin = 0x00;
const uint16_t syncWordMax = 0xFF;
const uint8_t sfMin = 7;
const uint8_t sfMax = 12;
const float bandwidths[] = {7.8, 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125.0, 250.0, 500.0};
const uint8_t bandwidthCount = sizeof(bandwidths) / sizeof(bandwidths[0]);
const uint8_t codingRates[] = {5, 6, 7, 8};
const uint8_t codingRateCount = sizeof(codingRates) / sizeof(codingRates[0]);
const bool crcEnabled[] = {true, false};
const uint8_t crcCount = sizeof(crcEnabled) / sizeof(crcEnabled[0]);
const bool explicitHeader[] = {true, false};
const uint8_t headerCount = sizeof(explicitHeader) / sizeof(explicitHeader[0]);
// --- FIX 2: Controllable Listening Time ---
const unsigned long listenInterval = 10000; 
const uint8_t FIXED_PACKET_LENGTH = 240;

// --- STATE VARIABLES ---
unsigned long prevMillis = 0;
bool messageFound = false;

float currentFreq = freqMin;
uint16_t currentSyncWord = syncWordMin;
uint8_t currentSF = sfMin;
uint8_t currentBWIndex = 0;
uint8_t currentCRIndex = 0;
uint8_t currentCRCIndex = 0;
uint8_t currentHeaderIndex = 0;

void applyRadioSettings() {
  radio.setSyncWord((uint8_t)currentSyncWord);
  radio.setSpreadingFactor(currentSF);
  radio.setBandwidth(bandwidths[currentBWIndex]);
  radio.setCodingRate(codingRates[currentCRIndex]);
  radio.setOutputPower(22);  // Keep high TX power for better sensitivity/receive
  radio.setCRC(crcEnabled[currentCRCIndex]);
  if (explicitHeader[currentHeaderIndex]) {
    radio.explicitHeader();
  } else {
    radio.implicitHeader(FIXED_PACKET_LENGTH);
  }
  Serial.print("Trying -> Freq: ");
  Serial.print(currentFreq, 3);
  Serial.print(" MHz, Sync: 0x");
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
  if (currentHeaderIndex < headerCount) { return; }
  currentHeaderIndex = 0;
  
  currentCRCIndex++;
  if (currentCRCIndex < crcCount) { return; }
  currentCRCIndex = 0;
  
  currentCRIndex++;
  if (currentCRIndex < codingRateCount) { return; }
  currentCRIndex = 0;
  
  currentBWIndex++;
  if (currentBWIndex < bandwidthCount) { return; }
  currentBWIndex = 0;

  currentSF++;
  if (currentSF <= sfMax) { return; }
  currentSF = sfMin;

  currentSyncWord++;
  if (currentSyncWord <= syncWordMax) { return; }
  currentSyncWord = syncWordMin;

  currentFreq += freqStep;
  if (currentFreq > freqMax) {
    currentFreq = freqMin;
    Serial.println("All combinations tried. Restarting scan.");
  }

  radio.setFrequency(currentFreq);
}

void test(){
  currentFreq = 851.125;           // Your target frequency
currentSyncWord = 0x12;          // Your sync word
currentSF = 9;                   // SF=9
currentBWIndex = 7;              // bandwidths[7] = 125 kHz
currentCRIndex = 0;              // coding rate = 5 (4/5)
currentCRCIndex = 0;             // CRC enabled
currentHeaderIndex = 0;          // Explicit header

}

void applyAndListen() {
  applyRadioSettings();
  radio.startReceive();
  prevMillis = millis();
}

void setup() {
  test();
  Serial.begin(115200);
  while (!Serial);
  delay(500);
  Serial.println("Starting RadioLib SX1262 Brute-force Scanner...");
  spiLoRa.begin(SCK, MISO, MOSI);

  int state = radio.begin(currentFreq);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("Radio init failed with code ");
    Serial.println(state);
    while (true);
  }
  applyAndListen();
}

void loop() {
  if (messageFound) {
    return;
  }

  // Use blocking receive with fixed packet length as in your working detection code
  String str;
  int state = radio.receive(str, FIXED_PACKET_LENGTH);

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
    
    messageFound = true;
    radio.standby();
    return;
  }
  else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
    if (millis() - prevMillis >= listenInterval) {
      radio.standby();
      Serial.println("Timeout, trying next combination...");
      nextCombination();
      applyAndListen();
    }
  }
  else {
    Serial.print("Receive failed with error code ");
    Serial.println(state);
    if (millis() - prevMillis >= listenInterval) {
      radio.standby();
      Serial.println("Error or no packet, trying next combination...");
      nextCombination();
      applyAndListen();
    }
  }
}
