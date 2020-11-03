#include "cbxpkt.h"

/**
 * Logs packet details over the USART line
 * 
 * @param pkt the packet to be logged
 */
void cbx_pkt_log(const cbx_pkt_t *pkt) {
  char sender[] = {"00:00:00:00:00:00\0"};
  char receiver[] = {"00:00:00:00:00:00\0"};

  /* Convers the addresses to strings */
  ieee80211_mac_to_string(receiver, pkt->hdr.receiver);
  ieee80211_mac_to_string(sender, pkt->hdr.sender);

  /* Prints the basic packet information */
  uint32_t flags = 0x0;
  memcpy(&flags, &pkt->hdr.flags, 1);
  Serial.printf(
    "cbx_pkt_t {\r\n"
    "\tHeader {\r\n"
    "\t\tLabel: %c%c%c%c\r\n"
    "\t\tSender: %s\r\n"
    "\t\tReceiver: %s\r\n"
    "\t\tChain no: %d\r\n"
    "\t\tFlags: %02X\r\n"
    "\t}\r\n"
    "\tBody {\r\n"
    "\t\tSize: %d\r\n"
    "\t}\r\n"
    "}\r\n"
    , // ==========================================
    pkt->hdr.label[0], pkt->hdr.label[1], pkt->hdr.label[2], pkt->hdr.label[3],
    sender,
    receiver,
    pkt->hdr.chain_no,
    flags,
    pkt->body.size
  );
}

/**
 * Transmits an packet over the LoRa antenna
 * 
 * @param pkt the packet to be transmitted
 */
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