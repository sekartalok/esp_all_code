#include <RadioLib.h>
#include <LittleFS.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>

// --- CONNECTIONS ---
#define NSS   7
#define RST   8
#define DIO1  33
#define BUSY  34
#define SCK   5
#define MISO  3
#define MOSI  6

// SD Card pins
#define SD_CS    13
#define SD_MOSI  11
#define SD_MISO  2
#define SD_SCLK  14

SPIClass spiLoRa(FSPI);
SPIClass spiSD(HSPI);
SX1262 radio = new Module(NSS, DIO1, RST, BUSY, spiLoRa);

// --- BUFFER STRUCTURE ---
struct DetectionResult {
  float frequency;
  uint16_t syncWord;
  uint8_t spreadingFactor;
  float bandwidth;
  uint8_t codingRate;
  bool crcEnabled;
  bool explicitHeader;
  int16_t rssi;
  float snr;
  uint8_t spikeCount;
  bool burstDetected;
  bool packetFound;
  String packetData;
  unsigned long timestamp;
};

// --- BUFFER MANAGEMENT ---
const uint8_t BUFFER_SIZE = 20;
DetectionResult resultBuffer[BUFFER_SIZE];
uint8_t bufferIndex = 0;
bool bufferFull = false;

// --- BRUTE-FORCE PARAMETERS ---
const float freqMin        = 150.0;
const float freqMax        = 960.0;
const float freqStep       = 0.2;
const uint16_t syncWordMin = 0x00;
const uint16_t syncWordMax = 0xFF;
const uint8_t sfMin        = 5;
const uint8_t sfMax        = 12;
const float bandwidths[]   = {7.8,10.4,15.6,20.8,31.25,41.7,62.5,125.0,250.0,500.0};
const uint8_t bwCount      = sizeof(bandwidths)/sizeof(bandwidths[0]);
const uint8_t crs[]        = {5,6,7,8};
const uint8_t crCount      = sizeof(crs)/sizeof(crs[0]);
const bool crcs[]          = {true,false};
const uint8_t crcCount     = sizeof(crcs)/sizeof(crcs[0]);
const bool hdrs[]          = {true,false};
const uint8_t hdrCount     = sizeof(hdrs)/sizeof(hdrs[0]);

// --- LISTEN PARAMETERS ---
const unsigned long listenInterval = 500; // ms per combo
const uint8_t FIXED_PKT_LEN        = 240;

// --- STATE ---
unsigned long prevMillis = 0;
bool burstDetected       = false;
bool pktFound            = false;

float currentFreq        = freqMin;
uint16_t currentSW       = syncWordMin;
uint8_t currentSF        = sfMin;
uint8_t currentBW_i      = 0;
uint8_t currentCR_i      = 0;
uint8_t currentCRC_i     = 0;
uint8_t currentHDR_i     = 0;

// baseline stats from initial 20 checks
int16_t baselineRSSI     = 0;
uint8_t baseSpikeCount   = 0;

void test(){
  currentFreq = 851.125;
  currentSW   = 0x12;
}

// --- BUFFER MANAGEMENT FUNCTIONS ---
void writeBufferToSD();

void addToBuffer(DetectionResult result) {
  resultBuffer[bufferIndex++] = result;

  if (bufferIndex >= BUFFER_SIZE) {
    bufferFull = true;
    writeBufferToSD();
    bufferIndex = 0;
  }
}

void writeBufferToSD() {
  if (!SD.exists("/logs")) {
    SD.mkdir("/logs");
  }
  File file = SD.open("/logs/detection_log.txt", "a");
  if (!file) {
    Serial.println("Failed to open SD card file");
    return;
  }
  // Write CSV header if file is new
  if (file.size() == 0) {
    file.println("Timestamp,Frequency,SyncWord,SF,Bandwidth,CodingRate,CRC,Header,RSSI,SNR,Spikes,BurstDetected,PacketFound,PacketData");
  }
  uint8_t endIndex = bufferFull ? BUFFER_SIZE : bufferIndex;
  for (uint8_t i = 0; i < endIndex; i++) {
    DetectionResult &r = resultBuffer[i];
    file.print(r.timestamp); file.print(',');
    file.print(r.frequency, 3); file.print(',');
    file.print("0x");
    if (r.syncWord < 0x10) file.print('0');
    file.print(r.syncWord, HEX); file.print(',');
    file.print(r.spreadingFactor); file.print(',');
    file.print(r.bandwidth, 1); file.print(',');
    file.print("4/"); file.print(r.codingRate); file.print(',');
    file.print(r.crcEnabled ? "On" : "Off"); file.print(',');
    file.print(r.explicitHeader ? "Exp" : "Imp"); file.print(',');
    file.print(r.rssi); file.print(',');
    file.print(r.snr, 1); file.print(',');
    file.print(r.spikeCount); file.print(',');
    file.print(r.burstDetected ? "YES" : "NO"); file.print(',');
    file.print(r.packetFound ? "YES" : "NO"); file.print(',');
    String clean = r.packetData;
    clean.replace(",", ";");
    clean.replace("\n", " ");
    clean.replace("\r", " ");
    file.println(clean);
  }
  file.close();
  Serial.print("Buffer written to SD card. Records: ");
  Serial.println(endIndex);
  bufferFull = false;
}

void flushBufferToSD() {
  if (bufferIndex > 0) {
    writeBufferToSD();
  }
}

void applySettings() {
  radio.setFrequency(currentFreq);
  radio.setSyncWord((uint8_t)currentSW);
  radio.setSpreadingFactor(currentSF);
  radio.setBandwidth(bandwidths[currentBW_i]);
  radio.setCodingRate(crs[currentCR_i]);
  radio.setCRC(crcs[currentCRC_i]);
  if (hdrs[currentHDR_i]) radio.explicitHeader();
  else                    radio.implicitHeader(FIXED_PKT_LEN);
  radio.setOutputPower(22);

  Serial.print("Try F:"); Serial.print(currentFreq,3);
  Serial.print(" SW:0x"); if (currentSW<0x10) Serial.print('0');
  Serial.print(currentSW, HEX);
  Serial.print(" SF:"); Serial.print(currentSF);
  Serial.print(" BW:"); Serial.print(bandwidths[currentBW_i]);
  Serial.print(" CR:4/"); Serial.print(crs[currentCR_i]);
  Serial.print(" CRC:"); Serial.print(crcs[currentCRC_i] ? "On":"Off");
  Serial.print(" Hdr:"); Serial.println(hdrs[currentHDR_i]?"Exp":"Imp");
}

void nextCombo() {
  currentHDR_i++;
  if (currentHDR_i < hdrCount) return; currentHDR_i = 0;
  currentCRC_i++;
  if (currentCRC_i < crcCount) return; currentCRC_i = 0;
  currentCR_i++;
  if (currentCR_i < crCount)   return; currentCR_i = 0;
  currentBW_i++;
  if (currentBW_i < bwCount)   return; currentBW_i = 0;
  currentSF++;
  if (currentSF <= sfMax)      return; currentSF = sfMin;
  currentSW++;
  if (currentSW <= syncWordMax)return; currentSW = syncWordMin;
  currentFreq += freqStep;
  if (currentFreq > freqMax) {
    currentFreq = freqMin;
    Serial.println("All combos done, restart.");
  }
}

void setup() {
  test();
  Serial.begin(115200);
  while (!Serial);
  delay(200);

  // Initialize SD card with custom SPI pins
  spiSD.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS, spiSD)) {
    Serial.println("SD Card initialization failed!");
    while (true);
  }
  Serial.println("SD Card initialized successfully");

  // Initialize LoRa SPI
  spiLoRa.begin(SCK, MISO, MOSI);
  if (radio.begin(currentFreq) != RADIOLIB_ERR_NONE) {
    Serial.println("LoRa Init fail");
    while (true);
  }

  // Initial baseline
  radio.startReceive();
  long sum = 0;
  baseSpikeCount = 0;
  for (int i = 0; i < 20; i++) {
    int16_t r = radio.getRSSI();
    sum += r;
    if (r > (sum/(i+1))) baseSpikeCount++;
    delay(5);
  }
  radio.standby();
  baselineRSSI = sum / 20;
  Serial.print("Baseline RSSI: "); Serial.print(baselineRSSI);
  Serial.print(" dBm, base spikes: "); Serial.println(baseSpikeCount);
  Serial.println("Starting brute-force with interval and spike threshold...");
}

void loop() {
  if (burstDetected || pktFound) {
    flushBufferToSD();
    return;
  }

  applySettings();
  prevMillis = millis();

  if (radio.startReceive() != RADIOLIB_ERR_NONE) {
    Serial.println("startReceive err");
    nextCombo();
    return;
  }
  delay(5);

  DetectionResult res;
  res.frequency      = currentFreq;
  res.syncWord       = currentSW;
  res.spreadingFactor= currentSF;
  res.bandwidth      = bandwidths[currentBW_i];
  res.codingRate     = crs[currentCR_i];
  res.crcEnabled     = crcs[currentCRC_i];
  res.explicitHeader = hdrs[currentHDR_i];
  res.timestamp      = millis();
  res.burstDetected  = false;
  res.packetFound    = false;
  res.spikeCount     = 0;

  uint8_t spikes = 0;
  while (millis() - prevMillis < listenInterval && spikes <= baseSpikeCount) {
    int16_t r = radio.getRSSI();
    if (r > baselineRSSI) spikes++;
    delay(5);
  }

  res.spikeCount = spikes;
  res.rssi       = radio.getRSSI();
  res.snr        = radio.getSNR();

  if (spikes > baseSpikeCount) {
    Serial.println("\n*** BURST DETECTED ***");
    Serial.print("Spikes: "); Serial.println(spikes);
    res.burstDetected = true;
    burstDetected = true;
  }
  else {
    String pkt;
    int st = radio.receive(pkt, FIXED_PKT_LEN);
    if (st == RADIOLIB_ERR_NONE) {
      Serial.println("\n*** PACKET DECODED ***");
      Serial.print("RSSI: "); Serial.print(radio.getRSSI()); Serial.println(" dBm");
      Serial.print("SNR:  "); Serial.print(radio.getSNR()); Serial.println(" dB");
      Serial.print("Data: "); Serial.println(pkt);
      res.packetFound = true;
      res.packetData = pkt;
      pktFound = true;
    }
  }

  addToBuffer(res);
  radio.standby();

  if (!burstDetected && !pktFound) {
    Serial.print("No detect, next combo... RSSI:"); Serial.println(radio.getRSSI());
    nextCombo();
  }
}
