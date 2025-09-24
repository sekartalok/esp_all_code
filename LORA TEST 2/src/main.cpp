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
// --- BRUTE-FORCE PARAMETERS FOR FSK ---
const float freqMin = 150.0;
const float freqMax = 960.0;
const float freqStep = 0.2;
// FSK-specific parameters
const float bitRates[] = {1.2, 2.4, 4.8, 9.6, 19.2, 38.4}; // kbps
const uint8_t bitRateCount = sizeof(bitRates) / sizeof(bitRates[0]);
const float freqDeviations[] = {5.0, 25.0, 50.0}; // kHz
const uint8_t freqDeviationCount = sizeof(freqDeviations) / sizeof(freqDeviations[0]);
const float rxBandwidths[] = {4.8, 5.8, 7.3, 9.7, 11.7, 14.6, 19.5, 23.4, 29.3, 39.0, 46.9, 58.6, 78.2, 93.8, 117.3, 156.2, 187.2, 234.3, 312.0, 373.6, 467.0}; // kHz
const uint8_t rxBandwidthCount = sizeof(rxBandwidths) / sizeof(rxBandwidths[0]);
// Sync word configurations
const uint16_t syncWordMin = 0x00;
const uint16_t syncWordMax = 0xFF;
// Preamble lengths
const uint8_t preambleLengths[] = {8, 16, 32}; // bits
const uint8_t preambleCount = sizeof(preambleLengths) / sizeof(preambleLengths[0]);
// --- TIMING ---
const unsigned long listenInterval = 5000; // Shorter interval for FSK
const uint8_t FIXED_PACKET_LENGTH = 240;
// --- STATE VARIABLES ---
unsigned long prevMillis = 0;
bool messageFound = false;
float currentFreq = freqMin;
uint16_t currentSyncWord = syncWordMin;
uint8_t currentBitRateIndex = 0;
uint8_t currentFreqDevIndex = 0;
uint8_t currentRxBWIndex = 0;
uint8_t currentPreambleIndex = 0;

void applyRadioSettings() {
  // Set FSK parameters
  radio.setBitRate(bitRates[currentBitRateIndex]);
  radio.setFrequencyDeviation(freqDeviations[currentFreqDevIndex]);
  radio.setRxBandwidth(rxBandwidths[currentRxBWIndex]);
  // Set sync word as array of bytes
  uint8_t syncBytes[2] = {(uint8_t)(currentSyncWord >> 8), (uint8_t)(currentSyncWord & 0xFF)};
  radio.setSyncWord(syncBytes, 2);
  radio.setOutputPower(22);  // Keep high TX power
  radio.setPreambleLength(preambleLengths[currentPreambleIndex]);
  
  // ALWAYS keep CRC enabled
  radio.setCRC(2); // 2-byte CRC for FSK - ALWAYS ON
  
  // Set fixed packet length
  radio.fixedPacketLengthMode(FIXED_PACKET_LENGTH);
  
  Serial.print("Trying FSK -> Freq: ");
  Serial.print(currentFreq, 3);
  Serial.print(" MHz, Sync: 0x");
  if (currentSyncWord < 0x10) Serial.print("0");
  Serial.print(currentSyncWord, HEX);
  Serial.print(", BR: ");
  Serial.print(bitRates[currentBitRateIndex]);
  Serial.print(" kbps, FreqDev: ");
  Serial.print(freqDeviations[currentFreqDevIndex]);
  Serial.print(" kHz, RxBW: ");
  Serial.print(rxBandwidths[currentRxBWIndex]);
  Serial.print(" kHz, Preamble: ");
  Serial.print(preambleLengths[currentPreambleIndex]);
  Serial.println(" bits, CRC: ON");
}

void nextCombination() {
  currentPreambleIndex++;
  if (currentPreambleIndex < preambleCount) { return; }
  currentPreambleIndex = 0;
  
  currentRxBWIndex++;
  if (currentRxBWIndex < rxBandwidthCount) { return; }
  currentRxBWIndex = 0;
  
  currentFreqDevIndex++;
  if (currentFreqDevIndex < freqDeviationCount) { return; }
  currentFreqDevIndex = 0;
  
  currentBitRateIndex++;
  if (currentBitRateIndex < bitRateCount) { return; }
  currentBitRateIndex = 0;
  
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

void test() {
  // Start with your known working parameters if needed
  currentFreq = 905.125;           // Your target frequency
  currentSyncWord = 0x12;          // Your sync word
}

void applyAndListen() {
  applyRadioSettings();
  radio.startReceive();
  prevMillis = millis();
}

void setup() {
  test(); // Start with known parameters
  Serial.begin(115200);
  while (!Serial);
  delay(500);
  Serial.println("Starting RadioLib SX1262 FSK Scanner...");
  Serial.println("CRC always ON - will ONLY stop when message received successfully (no error)");
  spiLoRa.begin(SCK, MISO, MOSI);
  // Initialize radio in FSK mode
  int state = radio.beginFSK(currentFreq, bitRates[currentBitRateIndex], freqDeviations[currentFreqDevIndex], rxBandwidths[currentRxBWIndex], 22, preambleLengths[currentPreambleIndex]);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("Radio FSK init failed with code ");
    Serial.println(state);
    while (true);
  }
  applyAndListen();
}

void loop() {
  if (messageFound) {
    return; // Stay stopped when message found
  }
  
  // Try to receive data
  String str;
  int state = radio.receive(str, FIXED_PACKET_LENGTH);
  
  if (state == RADIOLIB_ERR_NONE) {
    // SUCCESS - NO ERROR - STOP HERE!
    Serial.println("\n=========================================");
    Serial.println(">>> MESSAGE RECEIVED SUCCESSFULLY! <<<");
    Serial.print("Error code: ");
    Serial.println(state);
    Serial.println("Matching FSK configuration:");
    applyRadioSettings();
    Serial.print("Message RSSI: ");
    Serial.print(radio.getRSSI());
    Serial.println(" dBm");
    Serial.print("Message SNR: ");
    Serial.print(radio.getSNR());
    Serial.println(" dB");
    Serial.print("Message Length: ");
    Serial.println(str.length());
    Serial.print("Message Content: ");
    Serial.println(str);
    Serial.println("=========================================\n");
    
    messageFound = true;
    radio.standby();
    return;
  }
  else {
    // ANY ERROR - SHOW ERROR CODE BUT CONTINUE scanning
    if (millis() - prevMillis >= listenInterval) {
      Serial.print("Error code: ");
      Serial.println(state);
      radio.standby();
      Serial.println("Trying next combination...");
      nextCombination();
      applyAndListen();
    }
  }
}
