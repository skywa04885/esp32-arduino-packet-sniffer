#include "ieee80211.h"

/**
 * Turns an mac address into it's string representation
 * 
 * @param out the output buffer
 * @param in the input mac (6 bytes)
 */
void ieee80211_mac_to_string(char *out, const uint8_t *in) {
  sprintf(out, "%02x:%02x:%02x:%02x:%02x:%02x", in[0], in[1], in[2], in[3], in[4], in[5]);
}

/**
 * Gets the string version of ethernet frame type
 * 
 * @param type the type number
 */
const char *ieee80211_get_type_string(wifi_promiscuous_pkt_type_t type) {
  switch (type) {
    case WIFI_CF_MGMT: return "MGMT";
    case WIFI_CF_DATA: return "DATA";
    case WIFI_CF_CONTROL: return "CONTROL";
    default: return "Invalid/Ext";
  }
}

/**
 * Gets the string version of an management frame subtype
 * 
 * @param type the type to be converted
 */
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

/**
 * Gets the string version of an control frame subtype
 * 
 * @param type the type number
 */
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

/**
 * Logs an IEEE80211 frame to the USART line, this is used directly
 *  in the callback of an promiscous wifi mode
 * 
 * @param buffer the input data buffer of frame
 * @param type the type of the frame
 */
void ieee80211_log_packet(void *buffer, wifi_promiscuous_pkt_type_t type);
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
    promisc_pkt->rx_ctrl.rssi, pkt_data);
}