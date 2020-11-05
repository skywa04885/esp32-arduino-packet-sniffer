/*
  Copyright Cybox 2020 - Written by Luke A.C.A. Rieff
*/

#include "main_receiver.h"

#ifdef COMPILE_AS_RECEIVER

static volatile bool g_Connected = false;

void http_write_macs(uint8_t *mac_addrs, uint8_t addr_count, const char *api_key) {
  if (!g_Connected) {
    Serial.println("Refusing packet transmission: no WiFi connection");
    return;
  }

  uint64_t now = std::chrono::duration_cast<std::chrono::seconds>(
    std::chrono::system_clock::now().time_since_epoch()
  ).count(), last_seen = now, first_seen = last_seen - 60;

  /* Prepares the body for the HTTP message */
  cJSON *body = cJSON_CreateObject(),
    *body_d = cJSON_CreateArray();

  /* Adds the timestamp */
  cJSON_AddNumberToObject(body, "ts", now);

  /* Loops over the mac addresses, while we turn them into numbers 
   * and add them to the body data array */
  uint8_t *mac_ptr = mac_addrs;
  for (uint8_t i = 0; i < addr_count; ++i) {
    /* Converts the MAC address to an 64 bit unsigned integer, so it
     * can be sent to the api (kinda retarded this way, just do it binary) */
    uint64_t number = static_cast<uint64_t>(mac_ptr[0]);
    number |= (static_cast<uint64_t>(mac_ptr[1]) << 8);
    number |= (static_cast<uint64_t>(mac_ptr[2]) << 16);
    number |= (static_cast<uint64_t>(mac_ptr[3]) << 24);
    number |= (static_cast<uint64_t>(mac_ptr[4]) << 32);
    number |= (static_cast<uint64_t>(mac_ptr[5]) << 40);

    /* Creates the data array [mac, first_seen, last_seen] */
    cJSON *element = cJSON_CreateArray(),
      *mac = cJSON_CreateNumber(number),
      *first = cJSON_CreateNumber(first_seen),
      *last = cJSON_CreateNumber(last_seen);
    cJSON_AddItemToArray(element, mac);
    cJSON_AddItemToArray(element, first);
    cJSON_AddItemToArray(element, last);

    /* Inserts the data to the final array of data */
    cJSON_AddItemToArray(body_d, element);

    /* Goes to the next address in the list */
    mac_ptr += sizeof (uint8_t) * 6;
  }

  // /* Adds the data array to the body, and generates the JSON string */
  cJSON_AddItemToObject(body, "d", body_d);
  char *http_body = cJSON_Print(body);
  DEBUG_ONLY(Serial.printf("Sending data: '%s'\r\n", http_body));
  size_t http_body_len = strlen(http_body);
  if (http_body == NULL) Serial.println("cJSON_Print() failed !\r\n");

  /* Configures the HTTP Request, and initializes the client */
  esp_http_client_config_t config {
    .url = "https://drukteradar.cybox.nl/api/"
  };
  esp_http_client_handle_t handle = esp_http_client_init(&config);

  /* Initializes the HTTP Request, and sends the headers */
  esp_http_client_set_method(handle, HTTP_METHOD_PUT);
  esp_http_client_set_header(handle, "Authorization", api_key);
  esp_http_client_set_header(handle, "Content-Type", "application/json");
  esp_http_client_set_post_field(handle, http_body, http_body_len);
  esp_err_t err = esp_http_client_perform(handle);
  if (err != ESP_OK) {
    Serial.printf("esp_http_client_perform() failed: %d->%s\r\n", err, 
      esp_err_to_name(err));
  } else {
    DEBUG_ONLY(Serial.printf("API Performed { Status: %d, Length: %d }\r\n",
      esp_http_client_get_status_code(handle), 
      esp_http_client_get_content_length(handle)));
  }

  // /* Free's the memory */
  cJSON_Delete(body);
  esp_http_client_cleanup(handle);
}

/**
 * Handles ESP events
 * 
 * @param ctx the context or something
 * @param event the event that occured
 */
esp_err_t event_handler(void *ctx, system_event_t *event) {
  switch (event->event_id) {
    case SYSTEM_EVENT_STA_START:
      esp_wifi_connect();
      break;
    case SYSTEM_EVENT_STA_GOT_IP:
      /* Prints the IP address, after which we set the new network time server */
      Serial.printf("Received IP address: %s\r\n", 
        ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
      configTime(3600, 0, "0.nl.pool.ntp.org", "1.nl.pool.ntp.org", "2.nl.pool.ntp.org");
      g_Connected = true;
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      esp_wifi_connect();
      g_Connected = false;
      break;
    default: break;
  }

  return ESP_OK;
}

/**
  * Performs all the initialization code
 */
void setup() {
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  wifi_config_t wifi_cfg = {
    .sta = {
      { .ssid = GLOBAL_WIFI_SSID },
      { .password = GLOBAL_WIFI_PASS },
    },
  };

  /* Inits serial */
  Serial.begin(GLOBAL_USART_BAUD);

  /* Inits NVS */
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    nvs_flash_erase();
    err = nvs_flash_init();
  } 

  /* Initializes WiFi */
  tcpip_adapter_init();
  esp_event_loop_init(&event_handler, NULL);
  
  esp_wifi_init(&cfg);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_STA);

  esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_cfg);
  esp_wifi_start();
  
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
 * Performs the receiving
 */
static uint8_t packet_buffer[GLOBAL_RECEIVER_BUFFER_SIZE];
void loop() {
  /* Gets the size of the possible packet, after which we check
   * if the size is larger than zero, indicating an packet */
  int32_t packet_size = LoRa.parsePacket();
  if (packet_size <= 0) return;

  DEBUG_ONLY(Serial.printf("Received packet { RSSI: "
   "%d, SNR: %d, Size: %d } \r\n", LoRa.packetRssi(), 
   static_cast<int32_t>(LoRa.packetSnr() * 100), packet_size));

  /* Checks if the packet is too large for the current buffer */
  if (packet_size > GLOBAL_RECEIVER_BUFFER_SIZE) {
    DEBUG_ONLY(Serial.printf("Ignoring packet, due to extreme"
      "size of %d bytes\r\n", packet_size));
    return;
  }

  /* Reads the packet after which we start parsing the packet
   * into usefull data */
  for (int32_t i = 0; i < packet_size; ++i)
    packet_buffer[i] = LoRa.read();

  /* Parses the packet, and logs it to the console */
  const cbx_pkt_t *pkt = reinterpret_cast<cbx_pkt_t *>(packet_buffer);
  cbx_pkt_log(pkt);

  /* Checks if the packet is from cybox, if not ignore */
  if (
    pkt->hdr.label[0] != 'C' || pkt->hdr.label[1] != 'B' || 
    pkt->hdr.label[2] != 'X' || pkt->hdr.label[3] != 'L'
  ) {
    DEBUG_ONLY(Serial.println("Ignoring packet, not from Cybox .."));
    return;
  }

  /* Parses the measurements and logs them */
  uint8_t measurement_count = pkt->body.size / sizeof (measurement_t);
  uint8_t *measurement_pointer = (reinterpret_cast<uint8_t *>(packet_buffer) + sizeof (cbx_pkt_t)) - sizeof (uint8_t *);
  uint8_t *measurement_base_pointer = measurement_pointer;
  for (uint8_t i = 0; i < measurement_count; ++i) {
    const measurement_t *m = reinterpret_cast<measurement_t *>(measurement_base_pointer);  
    measurement_base_pointer += sizeof (measurement_t);

    /* Prints the mac address to the serial console, only if
     * debug is enabled tho */
    DEBUG_ONLY({
      char mac_buffer[] = {"00:00:00:00:00:00\0"};
      ieee80211_mac_to_string(mac_buffer, m->mac);
      Serial.printf("MAC Received: %s\r\n", mac_buffer);
    });    
  }

  /* Writes the measurements to the API */
  http_write_macs(measurement_pointer, measurement_count, pkt->body.api_key);

  delay(10);
}

#endif