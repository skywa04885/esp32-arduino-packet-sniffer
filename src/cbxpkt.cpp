#include "cbxpkt.h"

void cbx_pkt_log(const cbx_pkt_t *pkt) {
  char mac_tmp[] = {"00:00:00:00:00:00\0"};

  Serial.println("cbx_pkt_t {");

  /* Prints the header of the CBX PKT */
  Serial.println("\tHeader: {");
  Serial.printf("\t\tLabel: %c%c%c%c\n", pkt->hdr.label[0], 
    pkt->hdr.label[1], pkt->hdr.label[2], pkt->hdr.label[3]);

  ieee80211_mac_to_string(mac_tmp, pkt->hdr.sender);
  Serial.printf("\t\tSender: %s\n", mac_tmp);
  ieee80211_mac_to_string(mac_tmp, pkt->hdr.receiver);
  Serial.printf("\t\tReceiver: %s\n", mac_tmp);
  
  Serial.printf("\t\tChain no: %d\n", pkt->hdr.chain_no);
  Serial.printf("\t\tFlags: %02X\n", pkt->hdr.flags);
  Serial.println("\t}");

  /* Prints the body of the CBX packet */
  Serial.println("\tBody: {");

  Serial.printf("\t\tUnique ID: %d\n", pkt->body.unique_id);
  Serial.printf("\t\tSize: %d\n", pkt->body.size);
  Serial.printf("\t\tBody: ");

  for (uint8_t i = 0; i < pkt->body.size; ++i) {
    if (i > 0) Serial.write('-');
    Serial.printf("%02X", pkt->body.payload[i]);
  }

  Serial.println("\n\t}");
  Serial.println("}");
}

void cbx_pkt_transmit(const cbx_pkt_t *pkt) {
  const uint8_t *bin = nullptr;
  uint8_t write_size = 0, packet_size = 0;

  /* Begins the transmission of the packet */
  LoRa.beginPacket();

  /* Defines the anonymous function which will send some
   * bytes over lora */
  auto transmit = [&]() {
    packet_size += write_size;
    for (uint8_t i = 0; i < write_size; ++i) LoRa.write(bin[i]);
  };

  /* Sends the packet header, which will contain the flags etcetera
   * all of the size will be known at runtime */
  bin = reinterpret_cast<const uint8_t *>(&pkt->hdr);
  write_size = sizeof (cbx_pkt_hdr_t);
  transmit();

  /* Sends the packet body without the payload, since the
   * size is not know at compilation time, we will do this separately */
  bin = reinterpret_cast<const uint8_t *>(&pkt->body);
  write_size = sizeof (cbx_pkt_body_t) - sizeof (uint8_t *);
  transmit();

  /* Sends the payload, the size will not be known by default, the size is
   * defined in the body size attribute */
  write_size = pkt->body.size;
  bin = pkt->body.payload;
  transmit();

  /* Ends the packet */
  LoRa.endPacket();
}