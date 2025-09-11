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

int counter = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("LoRa Module Init...");
  spiLoRa.begin(SCK, MISO, MOSI, NSS);

  int state = radio.begin(850.125);
  Serial.print("radio.begin() = ");
  Serial.println(state);

  if (state != RADIOLIB_ERR_NONE) {
    Serial.println("Init failed - stopping.");
    while (true);
  }

  // Configure parameters (must match on both ends)
  radio.setBandwidth(125.0);
  radio.setSpreadingFactor(9);
  radio.setCodingRate(5);
  radio.setOutputPower(22);


  Serial.println("LoRa ready.");
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
    Serial.print("Contents: ");
    Serial.println(str);

    // Re-enable receive mode
    radio.startReceive();
  }
  else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
    // Nothing received, no error
  }
  else {
    Serial.print("Receive failed with error code ");
    Serial.println(state);
  }
}

// ----------- MAIN LOOP -----------
void loop() {
  // Uncomment one depending on role
  //sendMessage();     // For Transmitter
   receiveMessage();  // For Receiver
}
