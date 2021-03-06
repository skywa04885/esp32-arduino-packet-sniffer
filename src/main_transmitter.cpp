/*
  Copyright Cybox 2020 - Written by Luke A.C.A. Rieff
*/

#include "main_transmitter.h"

#ifndef COMPILE_AS_RECEIVER

/*******************************
 * Global variables
 ******************************/

/* Will be used to keep track of the measurements inside the program
 * the size is used to detect overflow and trigger transmission */
static measurement_t g_Measurements[GLOBAL_MEASUREMENT_BUFFER_SIZE];
static size_t g_MeasurementCounter = 0;
static int64_t g_LastTransmissionTime = 0;

/* The filter which will be applied to the promiscous wifi mode
 * this will only allow management frames */
static wifi_promiscuous_filter_t g_PromiscFilter = {
  .filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT
};

/* The channel is used for channel switching, while the transmitting boolean
 * indicates if we should ignore packets at that moment  */
static uint8_t channel = 1;
static bool transmitting = false;

/*******************************
 * Functions
 ******************************/

/**
 * Transmits the measurements currently buffered
 */
void lora_transmit_measurements() {
  transmitting = true;

  /* Defines the paykoad buffer, and the packet with the default
   * packet values .. */
  uint8_t payload_buffer[128];
  cbx_pkt_t packet = {
    .hdr = {
      .label = { 'C', 'B', 'X', 'L' },
      .sender = DEVICE_MAC,
      .receiver = GATEWAY_MAC,
      .chain_no = 0,
      .flags = {
        .encrypted = 0x1,
        .relayed = 0x0,
        .chained = 0x0
      },
    },
    .body = {
      .unique_id = 0x00000000,
      { .api_key = GLOBAL_API_KEY },
      .size = 0,
      .payload = payload_buffer
    }
  };

  /* Defines the anonymous function which will transfer the current
   * packet over LoRa */
  bool first_packet = true; /* If the current transmitted packet is the first */
  auto transmit_packet = [&]() {
    /* Checks if it is the first packet, if so just transmit, else we will
      * increment the chain id and set the chain flag */
    if (!first_packet) {
      DEBUG_ONLY(Serial.printf("Writing chained packet, "
        "with payload size of: %d\r\n", packet.body.size));
    
      /* Sets chained to true, and increments the chain ID */ 
      packet.hdr.flags.chained = 0x1;
      ++packet.hdr.chain_no;
    } else {
      first_packet = false;
      DEBUG_ONLY(Serial.printf("Writing non-chained packet, "
        "with payload size of: %d\r\n", packet.body.size));
    }

    /* Transmits the packet over lora, after which we reset
      * the body size, in order to continue with the next elements */
    DEBUG_ONLY(cbx_pkt_log(&packet));
    cbx_pkt_transmit(&packet);
    packet.body.size = 0;
  };
  
  /* Starts looping over all the measurements, and sending the packets
   * with the corrent payload */
  for (uint8_t i = 0; i < g_MeasurementCounter; ++i) {
    const measurement_t *m = &g_Measurements[i];

    /* Checks if the current measurement fits into the
     * payload of the packet, if not transmit it first */
    if ((packet.body.size + sizeof (measurement_t)) > sizeof (payload_buffer))
      transmit_packet();

    /* Copies the measurement into the payload buffer, after which we append
     * an new measurement to the total size of the packet */
    memcpy(&payload_buffer[packet.body.size], m, sizeof (measurement_t));
    packet.body.size += sizeof (measurement_t);
  }

  /* Checks if there is any data left to be transmitted, if so
   * transmit it */
  if (packet.body.size > 0)
    transmit_packet();

  /* Resets the transmission time */
  g_LastTransmissionTime = esp_timer_get_time();
  g_MeasurementCounter = 0;
  transmitting = false;
}

/**
 * The callback for incomming promiscous packets
 * 
 * @param buffer the buffer which contains the data of the packet
 * @param type the type of packet
 */
void promisc_packet_cb(void *buffer, wifi_promiscuous_pkt_type_t type)  {
  if (transmitting) return;
  
  /* Gets the required information for the current measurement */
  measurement_t m;
  wifi_promiscuous_pkt_t *promisc_pkt = (wifi_promiscuous_pkt_t *) buffer;
  switch (type) {
    case WIFI_CF_MGMT: {
      ieee80211_management_packet_t *pkt = (ieee80211_management_packet_t *) promisc_pkt->payload;
      memcpy(m.mac, pkt->hdr.transmitter, 6);
      break;
    }
    case WIFI_CF_CONTROL: {
      ieee80211_control_mac_header_t *hdr = (ieee80211_control_mac_header_t *) promisc_pkt->payload;
      memcpy(m.mac, hdr->transmitter, 6);
      break;
    }
    case WIFI_CF_DATA: {
      ieee80211_data_packet_t *pkt = (ieee80211_data_packet_t *) promisc_pkt->payload;
      memcpy(m.mac, pkt->hdr.address2, 6);
      break;
    }
    case WIFI_CF_EXT: return;
    default: break;
  }

  /* Stores the measurement in the list of measurements, if full we will
   * start the transmission */
  for (uint16_t i = 0; i < g_MeasurementCounter; ++i) {
    measurement_t *mi = &g_Measurements[i];
    if (mi->mac[0] != m.mac[0]) continue;
    else if (mi->mac[1] != m.mac[1]) continue;
    else if (mi->mac[2] != m.mac[2]) continue;
    else if (mi->mac[3] != m.mac[3]) continue;
    else if (mi->mac[4] != m.mac[4]) continue;
    else if (mi->mac[5] != m.mac[5]) continue;
    else return;
  }

  DEBUG_ONLY({
    char mac[] = {"00:00:00:00:00:00\0"};
    ieee80211_mac_to_string(mac, m.mac);
    Serial.printf("Unique mac: %s\r\n", mac);
  });
  
  g_Measurements[g_MeasurementCounter++] = m;
  if (g_MeasurementCounter >= GLOBAL_MEASUREMENT_BUFFER_SIZE) {
    lora_transmit_measurements();
  }
}

/**
  * Performs all the initialization code
 */
void setup() {
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  
  /* Inits serial */
  Serial.begin(GLOBAL_USART_BAUD);

  /* Inits NVS */
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    nvs_flash_erase();
    err = nvs_flash_init();
  } 

  /* Inits WiFi */
  esp_event_loop_create_default();
  esp_wifi_init(&cfg);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_NULL);
  esp_wifi_start();

  /* Sets promiscous mode */
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_filter(&g_PromiscFilter);
  esp_wifi_set_promiscuous_rx_cb(&promisc_packet_cb);

  /* Initializes LoRa */
  SPI.begin(SCK,MISO,MOSI,SS);
  LoRa.setPins(SS,RST,DI0);
  if (!LoRa.begin(BAND)) {
    Serial.println("LoRa.begin() failed");
    for (;;);
  }

  Serial.println("LoRa.begin() succeeded");
}

/**
 * Switches the channels
 */
void loop() {
  /* Checks if the transmission time was to long ago, if so do it now*/
  if (esp_timer_get_time() > g_LastTransmissionTime + 60000000) {
    lora_transmit_measurements();
    return;
  }

  /* Performs the channel switching */
  if (channel > 11) channel = 1;
  else ++channel;
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  delay(GLOBAL_CHANNEL_SWITCH_DELAY);
}

#endif