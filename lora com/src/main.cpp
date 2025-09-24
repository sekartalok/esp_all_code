#include <RadioLib.h>

// --- CONNECTIONS ---
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
const float freqMin       = 150.0;
const float freqMax       = 960.0;
const float freqStep      = 0.2;
const uint16_t syncWordMin= 0x00;
const uint16_t syncWordMax= 0xFF;
const uint8_t sfMin       = 7;
const uint8_t sfMax       = 12;
const float bandwidths[]  = {7.8,10.4,15.6,20.8,31.25,41.7,62.5,125.0,250.0,500.0};
const uint8_t bwCount     = sizeof(bandwidths)/sizeof(bandwidths[0]);
const uint8_t crs[]       = {5,6,7,8};
const uint8_t crCount     = sizeof(crs)/sizeof(crs[0]);
const bool  crcs[]        = {true,false};
const uint8_t crcCount    = sizeof(crcs)/sizeof(crcs[0]);
const bool  hdrs[]        = {true,false};
const uint8_t hdrCount    = sizeof(hdrs)/sizeof(hdrs[0]);

// --- LISTEN PARAMETERS ---
const unsigned long listenInterval = 10000; // ms per combo
const uint8_t FIXED_PKT_LEN        = 240;

// --- STATE ---
unsigned long prevMillis = 0;
bool burstDetected        = false;
bool pktFound             = false;

float currentFreq         = freqMin;
uint16_t currentSW        = syncWordMin;
uint8_t currentSF         = sfMin;
uint8_t currentBW_i       = 0;
uint8_t currentCR_i       = 0;
uint8_t currentCRC_i      = 0;
uint8_t currentHDR_i      = 0;

// baseline stats from initial 20 checks
int16_t baselineRSSI      = 0;
uint8_t baseSpikeCount    = 0;

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
  Serial.print(" SW:0x"); if(currentSW<0x10)Serial.print('0');
  Serial.print(currentSW,HEX);
  Serial.print(" SF:"); Serial.print(currentSF);
  Serial.print(" BW:"); Serial.print(bandwidths[currentBW_i]);
  Serial.print(" CR:4/");Serial.print(crs[currentCR_i]);
  Serial.print(" CRC:");Serial.print(crcs[currentCRC_i]?"On":"Off");
  Serial.print(" Hdr:");Serial.println(hdrs[currentHDR_i]?"Exp":"Imp");
}

void nextCombo() {
  currentHDR_i++;
  if(currentHDR_i<hdrCount) return; currentHDR_i=0;
  currentCRC_i++;
  if(currentCRC_i<crcCount) return; currentCRC_i=0;
  currentCR_i++;
  if(currentCR_i<crCount)   return; currentCR_i=0;
  currentBW_i++;
  if(currentBW_i<bwCount)   return; currentBW_i=0;
  currentSF++;
  if(currentSF<=sfMax)      return; currentSF=sfMin;
  currentSW++;
  if(currentSW<=syncWordMax)return; currentSW=syncWordMin;
  currentFreq+=freqStep;
  if(currentFreq>freqMax){
    currentFreq=freqMin;
    Serial.println("All combos done, restart.");
  }
}

void setup() {
  Serial.begin(115200);
  while(!Serial);
  delay(200);

  spiLoRa.begin(SCK, MISO, MOSI);
  if(radio.begin(currentFreq)!=RADIOLIB_ERR_NONE){
    Serial.println("Init fail"); while(true);
  }

  // --- initial 20-check baseline ---
  radio.startReceive();
  long sum = 0;
  baseSpikeCount = 0;
  for(int i=0; i<20; i++){
    int16_t r = radio.getRSSI();
    sum += r;
    if(r > (sum/(i+1))) baseSpikeCount++;
    delay(5);
  }
  radio.standby();
  baselineRSSI = sum/20;
  Serial.print("Baseline RSSI: "); Serial.print(baselineRSSI);
  Serial.print(" dBm, base spikes: "); Serial.println(baseSpikeCount);

  Serial.println("Starting brute-force with interval and spike threshold...");
}

void loop() {
  if(burstDetected||pktFound) return;

  applySettings();
  prevMillis = millis();

  if(radio.startReceive()!=RADIOLIB_ERR_NONE){
    Serial.println("startReceive err");
    nextCombo();
    return;
  }
  delay(5); // settle

  // listen for spikes > baselineRSSI
  uint8_t spikeCount = 0;
  while(millis()-prevMillis<listenInterval && spikeCount <= baseSpikeCount){
    int16_t r = radio.getRSSI();
    if(r > baselineRSSI){
      spikeCount++;
    }
    delay(5);
  }

  if(spikeCount > baseSpikeCount){
    Serial.println("\n*** BURST DETECTED ***");
    Serial.print("Spikes: "); Serial.println(spikeCount);
    burstDetected = true;
  }
  else {
    // try decode
    String pkt;
    int st = radio.receive(pkt, FIXED_PKT_LEN);
    if(st==RADIOLIB_ERR_NONE){
      Serial.println("\n*** PACKET DECODED ***");
      Serial.print("RSSI: "); Serial.print(radio.getRSSI()); Serial.println(" dBm");
      Serial.print("SNR:  "); Serial.print(radio.getSNR()); Serial.println(" dB");
      Serial.print("Data: "); Serial.println(pkt);
      pktFound = true;
    }
  }

  radio.standby();
  if(!burstDetected && !pktFound){
    Serial.println("No detect, next combo...");
    nextCombo();
  }
}
