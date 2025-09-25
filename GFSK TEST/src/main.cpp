

#include <RadioLib.h>

// LoRa Module pins
#define NSS  7
#define RST  8
#define DIO1 33
#define BUSY 34
#define SCK  5
#define MISO 3
#define MOSI 6

SPIClass spiLoRa(FSPI);
SX1262 radio = new Module(NSS, DIO1, RST, BUSY, spiLoRa);

int counter = 0;

// ----------- SETUP -----------
void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("LoRa Module Init...");

  // Initialize SPI (do NOT include NSS, RadioLib handles NSS)
  spiLoRa.begin(SCK, MISO, MOSI);

  // Initialize radio at frequency only
  int state = radio.beginFSK(851.125);  // Only frequency set here
  Serial.print("radio.beginFSK() = ");
  Serial.println(state);

  if (state != RADIOLIB_ERR_NONE) {
    Serial.println("Init failed - stopping.");
    while (true);
  }

  // Manually configure all needed parameters to match your requirements
  radio.setBitRate(2.4);
  radio.setFrequencyDeviation(5.0);
  radio.setOutputPower(10);
  radio.setRxBandwidth(156.2);

  uint8_t syncWord[] = {0x12};  // Match E22 NET ID = 0x12
  radio.setSyncWord(syncWord, sizeof(syncWord));

  radio.setCRC(false);           // Enable CRC checking
  radio.setPreambleLength(8);  // Standard preamble length (in bits)

  radio.setDataShaping(RADIOLIB_SHAPING_1_0);
  Serial.println("Radio configured. Starting receiver...");
  radio.startReceive();         // Enter receive mode
}

// ----------- SEND FUNCTION -----------
void sendMessage() {
  String msg = "ESP32->Dongle#" + String(counter++);
  int state = radio.transmit(msg);

  Serial.print("TX state: ");
  Serial.println(state);

  delay(3000);  // wait before next send
}

// ----------- RECEIVE FUNCTION -----------
void receiveMessage() {
  String str;
  int state = radio.receive(str);

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("Packet received!");
    Serial.print("  Contents: ");
    Serial.println(str);

    Serial.print("  RSSI: ");
    Serial.print(radio.getRSSI());
    Serial.println(" dBm");

    radio.startReceive();       // Re-enable receive mode for next packet
  }
  else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
    // No packet received, no error
  }
  else {
    Serial.print("Receive failed with error code ");
    Serial.println(state);
  }
}

// ----------- MAIN LOOP -----------
void loop() {
  // Uncomment based on device role
  // sendMessage();  // For transmitter
  receiveMessage(); // For receiver
}