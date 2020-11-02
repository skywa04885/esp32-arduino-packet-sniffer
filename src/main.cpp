#include <Arduino.h>
#include <LoRa.h>

#include <esp_wifi.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <esp_task_wdt.h>
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"

#define SCK     5
#define MISO    19
#define MOSI    27
#define SS      18
#define RST     14
#define DI0     26
#define BAND    868E6

/**************************************************************
 * ieee80211.c
 **************************************************************/

typedef enum {
  WIFI_CF_MGMT,
  WIFI_CF_CONTROL,
  WIFI_CF_DATA,
  WIFI_CF_EXT
} ieee80211_control_frame_type_t;

typedef enum {
  WIFI_ASSOC_REQ,
  WIFI_ASSOC_RES,
  WIFI_REASSOC_REQ,
  WIFI_REASSOC_RES,
  WIFI_PROBE_REQ,
  WIFI_PROBE_RES,
  __WIFI_RESERVED0,
  __WIFI_RESERVED1,
  WIFI_BEACON,
  WIFI_ATIM,
  WIFI_DISASSOC,
  WIFI_AUTH,
  WIFI_DEAUTH,
  WIFI_ACTION,
  WIFI_ACTION_NO_ACK
} ieee80211_control_mgmt_subtype_t;

typedef enum {
  __WIFI_RESERVED2,
  __WIFI_RESERVED3,
  WIFI_TRIGGER,
  WIFI_REPORT_POLL,
  WIFI_NDP_ANNOUNCE,
  WIFI_CONTROL_FRAME_EXTENSION,
  WIFI_CONTROL_WRAPPER,
  WIFI_BLOCK_ACK_REQUEST,
  WIFI_BLOCK_ACK,
  WIFI_PS_POLL,
  WIFI_RTS,
  WIFI_CTS,
  WIFI_ACK,
  WIFI_CF_END,
  WIFI_CF_END__CF_ACK
} ieee80211_control_ctr_subtype_t;

typedef struct __attribute__ ((packed)) {
  unsigned protocol : 2;
  unsigned type : 2;
  unsigned subtype : 4;
  unsigned to_ds : 1;
  unsigned from_ds : 1;
  unsigned more_frag : 1;
  unsigned retry : 1;
  unsigned pwr_mgmt : 1;
  unsigned more_data : 1;
  unsigned protected_frame : 1;
  unsigned order : 1;
} ieee80211_control_frame_t;

typedef struct __attribute__ ((packed)) {
  ieee80211_control_frame_t cf;
  uint16_t duration;
  uint8_t address1[6];
  uint8_t address2[6];
  uint8_t address3[6];
  uint16_t seq_ctl;
  uint8_t address4[6];
} ieee80211_data_mac_header_t;

typedef struct __attribute__ ((packed)) {
  ieee80211_data_mac_header_t hdr;
  char payload[0];
} ieee80211_data_packet_t;

typedef struct __attribute__ ((packed)) {
  ieee80211_control_frame_t cf;
  uint16_t duration;
  uint8_t destination[6];
  uint8_t transmitter[6];
  uint8_t bssid[6];
  uint16_t seq_ctl;
} ieee80211_management_mac_header_t;

typedef struct __attribute__ ((packed)) {
  ieee80211_management_mac_header_t hdr;
  char payload[0];
} ieee80211_management_packet_t;

typedef struct __attribute__ ((packed)) {
  uint8_t timestamp[8];
  uint16_t interval;
  uint16_t capability;
  uint8_t tag_number;
  uint8_t tag_length;
  char ssid[0];
} ieee80211_management_beacon_var_t;

typedef struct __attribute__ ((packed)) {
  ieee80211_control_frame_t cf;
  uint16_t duration;
  uint8_t destination[6];
  uint8_t transmitter[6];
} ieee80211_control_mac_header_t;

void ieee80211_mac_to_string(char *out, uint8_t *in) {
  sprintf(out, "%02x:%02x:%02x:%02x:%02x:%02x", in[0], in[1], in[2], in[3], in[4], in[5]);
}

const char *ieee80211_get_type_string(wifi_promiscuous_pkt_type_t type) {
  switch (type) {
    case WIFI_CF_MGMT: return "MGMT";
    case WIFI_CF_DATA: return "DATA";
    case WIFI_CF_CONTROL: return "CONTROL";
    default: return "Invalid/Ext";
  }
}

const char *ieee80211_get_mgmt_subtype_string(ieee80211_control_mgmt_subtype_t type) {
  switch (type) {
    case WIFI_ASSOC_REQ: return "MGMT: AssocReq";
    case WIFI_ASSOC_RES: return "MGMT: AssocRes";
    case WIFI_REASSOC_REQ: return "MGMT: ReassocReq";
    case WIFI_REASSOC_RES: return "MGMT: ReassocRes";
    case WIFI_PROBE_REQ: return "MGMT: ProbeReq";
    case WIFI_PROBE_RES: return "MGMT: ProbeRes";
    case WIFI_BEACON: return "MGMT: Beacon";
    case WIFI_ATIM: return "MGMT: ATIM";
    case WIFI_DISASSOC: return "MGMT: DisAssoc";
    case WIFI_AUTH: return "MGMT: Auth";
    case WIFI_DEAUTH: return "MGMT: DeAuth";
    case WIFI_ACTION: return "MGMT: Action";
    case WIFI_ACTION_NO_ACK: return "MGMT: Action NoAck";
    default: return "MGMT: Invalid/Ext";
  }
}

const char *ieee80211_get_ctrl_subtype_string(ieee80211_control_ctr_subtype_t type) {
  switch (type) {
    case WIFI_TRIGGER: return "CTRL: Trigger";
    case WIFI_REPORT_POLL: return "CTRL: ReportPoll";
    case WIFI_NDP_ANNOUNCE: return "CTRL: Announce";
    case WIFI_CONTROL_FRAME_EXTENSION: return "CTRL: CTRLExt";
    case WIFI_CONTROL_WRAPPER: return "CTRL: CTRLWrapper";
    case WIFI_BLOCK_ACK_REQUEST: return "CTRL: BlockAckReq";
    case WIFI_BLOCK_ACK: return "CTRL: BlockAck";
    case WIFI_PS_POLL: return "CTRL: PSPoll";
    case WIFI_RTS: return "CTRL: RTS";
    case WIFI_CTS: return "CTRL: CTS";
    case WIFI_ACK: return "CTRL: ACK";
    case WIFI_CF_END: return "CTRL: CFEnd";
    case WIFI_CF_END__CF_ACK: return "CTRL: CFEnd&CFAck";
    default: return "CTRL: Invalid/ext";
  }
}

void ieee80211_log_packet(void *buffer, wifi_promiscuous_pkt_type_t type) {
  wifi_promiscuous_pkt_t *promisc_pkt = (wifi_promiscuous_pkt_t *) buffer;
  ieee80211_control_frame_t *cf = (ieee80211_control_frame_t *) promisc_pkt->payload;

  char pkt_destination_mac[] = {"00:00:00:00:00:00\0"};
  char pkt_transmitter_mac[] = {"00:00:00:00:00:00\0"};
  char pkt_data[32] = {'\0'};
  const char *label = ieee80211_get_type_string(type);

  // Switches the type, so we can parse the packet with the correct stucture
  //  and read the required data from it
  switch (type) {
    // ======================
    case WIFI_CF_MGMT: {
      ieee80211_management_packet_t *pkt = (ieee80211_management_packet_t *) promisc_pkt->payload;
      label = ieee80211_get_mgmt_subtype_string(static_cast<ieee80211_control_mgmt_subtype_t>(cf->subtype));

      // Turns the mac addresses in the management frame to strings,
      //  so we can log them to the console later on
      ieee80211_mac_to_string(pkt_transmitter_mac, pkt->hdr.transmitter);
      ieee80211_mac_to_string(pkt_destination_mac, pkt->hdr.destination);

      // Switches the subtype, to check if we can get any data from the packet
      //  since not all contain data
      switch (cf->subtype) {
        case WIFI_BEACON: case WIFI_PROBE_RES: {
          ieee80211_management_beacon_var_t *var = (ieee80211_management_beacon_var_t *) pkt->payload;

          if (var->tag_length > 31) strncpy(pkt_data, var->ssid, 31);
          else strncpy(pkt_data, var->ssid, var->tag_length);
          break;
        }
        default: break;
      }
      break;
    }
    // ======================
    case WIFI_CF_CONTROL: {
      ieee80211_control_mac_header_t *hdr = (ieee80211_control_mac_header_t *) promisc_pkt->payload;
      label = ieee80211_get_ctrl_subtype_string(static_cast<ieee80211_control_ctr_subtype_t>(cf->subtype));
      ieee80211_mac_to_string(pkt_transmitter_mac, hdr->destination);
      ieee80211_mac_to_string(pkt_destination_mac, hdr->transmitter);
      break;
    }
    // ======================
    case WIFI_CF_DATA: {
      ieee80211_data_packet_t *pkt = (ieee80211_data_packet_t *) promisc_pkt->payload;
      ieee80211_mac_to_string(pkt_transmitter_mac, pkt->hdr.address2);
      ieee80211_mac_to_string(pkt_destination_mac, pkt->hdr.address1);
      break;
    }
    // ======================
    case WIFI_CF_EXT: {
      break;
    }
    // ======================
    default: break;
  }

  // Prints the packet to the serial console, of course this happens in a decently
  //  formatted way: [ channel, size, transmitter, destination, type, freq, rssi ]
  Serial.printf("%-2u | %-5u | %s | %s | %-20s | %-3d DBM | '%s'\n", promisc_pkt->rx_ctrl.channel, promisc_pkt->rx_ctrl.sig_len, 
    pkt_transmitter_mac, pkt_destination_mac, label,
    promisc_pkt->rx_ctrl.rssi, pkt_data);;
}

/**************************************************************
 * main.c
 **************************************************************/

typedef struct {
  uint8_t mac[6];
} measurement_t;

static measurement_t g_Measurements[16];
static size_t g_MeasurementCounter = 0;

static wifi_promiscuous_filter_t g_PromiscFilter = {
  .filter_mask = WIFI_PROMIS_FILTER_MASK_ALL
};

static uint8_t channel = 1, n = 0;

#define LORA_TRANSMIT_MEASUREMENTS_TAG "TransmitMeasurements"
void lora_transmit_measurements() {
  esp_wifi_set_promiscuous(false);

  // Starts looping over the measurement, and transmitting them
  //  over lora, while keeping track of the max packet size
  size_t packetSize = 0, packetMeasurementCount = 0;
  for (measurement_t m : g_Measurements) {
    if (packetSize == 0) {
      LoRa.beginPacket();
      packetSize += LoRa.print("CBX_LPKT{");
      Serial.println("Begin packet");
    } else if (packetSize < 210) { // -> Append next measurement
      uint64_t mac = 0;
      for (uint8_t i = 0; i < 6; ++i) {
        if (i != 0) mac <<= 8;
        mac |= (uint64_t) m.mac[i];
      }

      const char *fmt = ":%lu";
      if (packetMeasurementCount++ == 0) fmt = "%lu";
      packetSize += LoRa.printf(fmt, mac);
      Serial.println("Data");
    } else { // -> Finish packet
      LoRa.print("}\n");
      LoRa.endPacket();
      packetSize = packetMeasurementCount = 0;
      Serial.println("End packet");
    }
  }

  if (packetMeasurementCount > 0) {
    LoRa.print("}\n");
    LoRa.endPacket();
    packetSize = packetMeasurementCount = 0;
    Serial.println("End packet");
  }

  // Clears the list of measurements
  g_MeasurementCounter = 0;
  esp_wifi_set_promiscuous(true);
}

void promisc_packet_cb(void *buffer, wifi_promiscuous_pkt_type_t type)  {
  if (++n > 20) {
    n = 0;

    if (channel == 1) channel = 6;
    else if (channel = 6) channel = 11;
    else channel = 1;

    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  }

  // ieee80211_log_packet(buffer, type);

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

  // /* Stores the measurement in the list of measurements, if full we will
  //  * start the transmission */
  g_Measurements[g_MeasurementCounter++] = m;
  if (g_MeasurementCounter >= 16)
    lora_transmit_measurements();
}

void setup() {
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  
  /* Inits serial */
  Serial.begin(38400);

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

void loop() {
  delay(10000);
}