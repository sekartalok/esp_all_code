#include <Arduino.h>
#include <ESP32DMASPIMaster.h>
#include "helper.h"
#include "GPIOIMC.h"  // defines SCL, SDA, NCS, ADO, etc.

static constexpr size_t BUFFER_SIZE = 1;
static constexpr size_t QUEUE_SIZE  = 256;

ESP32DMASPI::Master master;
uint8_t* dma_tx_buf;
uint8_t* dma_rx_buf;

volatile static size_t count_pre_cb  = 0;
volatile static size_t count_post_cb = 0;

void IRAM_ATTR userTransactionCallback(spi_transaction_t* trans, void* arg) {
  size_t* counter = (size_t*)arg;
  *counter += 1;
}

void setup() {
  Serial.begin(115200);
  delay(500);

  dma_tx_buf = master.allocDMABuffer(BUFFER_SIZE);
  dma_rx_buf = master.allocDMABuffer(BUFFER_SIZE);

  master.setDataMode(SPI_MODE3);
  master.setFrequency(ONE_MHZ);
  master.setMaxTransferSize(BUFFER_SIZE);
  master.setQueueSize(QUEUE_SIZE);
  master.begin(HSPI, SCL, SDA, ADO, NCS);
  delay(10);

  master.setUserPreCbAndArg(userTransactionCallback,  (void*)&count_pre_cb);
  master.setUserPostCbAndArg(userTransactionCallback, (void*)&count_post_cb);

  // Select Bank 0 (write 0 to REG_BANK_SEL)
  dma_tx_buf[0] = (0x7F & 0x7F); // write
  dma_tx_buf[1] = 0x00;          // bank 0
  master.transfer(dma_tx_buf, dma_rx_buf, 2);
  delay(10);

  Serial.println("ICM-20948 DMA SPI master ready");
}

void loop() {
  // Every second, read WHO_AM_I at 0x00
  static uint32_t last = 0;
  if (millis() - last > 1000 && master.numTransactionsInFlight() == 0) {
    last = millis();

    // Read command for 0x00
    dma_tx_buf[0] = 0x00 | 0x80; // MSB=1 for read
    dma_tx_buf[1] = 0x00;        // dummy

    master.queue(dma_tx_buf, dma_rx_buf, 2);
    master.trigger();
    while (master.numTransactionsInFlight() > 0) { }

    uint8_t whoami = dma_rx_buf[1];
    Serial.printf("ICM-20948 WHO_AM_I = 0x%02X\n", whoami);
  }
}
