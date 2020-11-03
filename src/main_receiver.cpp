/*
  Copyright Cybox 2020 - Written by Luke A.C.A. Rieff
*/

#include "main_receiver.h"

#ifdef COMPILE_AS_RECEIVER

/**
  * Performs all the initialization code
 */
void setup() {
  /* Inits serial */
  Serial.begin(GLOBAL_USART_BAUD);

  /* Initializes LoRa */
  SPI.begin(SCK,MISO,MOSI,SS);
  LoRa.setPins(SS,RST,DI0);
  if (!LoRa.begin(BAND)) {
    Serial.println("LoRa.begin() failed");
    for (;;);
  }
}

/**
 * Performs the receiving
 */
static uint8_t packet_buffer[GLOBAL_RECEIVER_BUFFER_SIZE];
void loop() {
  /* Gets the size of the possible packet, after which we check
   * if the size is larger than zero, indicating an packet */
  int32_t packet_size = LoRa.parsePacket();
  if (packet_size <= 0) return;

  Serial.printf("Received packet { RSSI: %d, SNR: %d, Size: %d } \r\n", LoRa.packetRssi(), LoRa.packetSnr(), packet_size);

  /* Checks if the packet is too large for the current buffer */
  if (packet_size > GLOBAL_RECEIVER_BUFFER_SIZE) {
    Serial.printf("Ignoring packet, due to extreme size of %d bytes\r\n", packet_size);
    return;
  }

  /* Reads the packet after which we start parsing the packet
   * into usefull data */
  for (int32_t i = 0; i < packet_size; ++i)
    packet_buffer[i] = LoRa.read();

  /* Parses the packet, and logs it to the console */
  const cbx_pkt_t *pkt = reinterpret_cast<cbx_pkt_t *>(packet_buffer);
  cbx_pkt_log(pkt);

  /* Parses the measurements and logs them */
  char mac_buffer[] = {"00:00:00:00:00:00\0"};
  uint8_t measurement_count = pkt->body.size / sizeof (measurement_t);
  uint8_t *measurement_base_pointer = (reinterpret_cast<uint8_t *>(packet_buffer) + sizeof (cbx_pkt_t)) - sizeof (uint8_t *);
  for (uint8_t i = 0; i < measurement_count; ++i) {
    const measurement_t *m = reinterpret_cast<measurement_t *>(measurement_base_pointer);  

    ieee80211_mac_to_string(mac_buffer, m->mac);
    Serial.printf("MAC Received: %s\r\n", mac_buffer);
    
    measurement_base_pointer += sizeof (measurement_t);
  }

  delay(10);
}

#endif