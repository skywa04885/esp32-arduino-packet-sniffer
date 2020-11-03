/*
  Copyright Cybox 2020 - Written by Luke A.C.A. Rieff
*/

#ifndef _MAIN_TRANSMITTER_H
#define _MAIN_TRANSMITTER_H

#include "default.h"
#include "ieee80211.h"
#include "cbxpkt.h"

#ifndef COMPILE_AS_RECEIVER

/*******************************
 * Function prototypes
 ******************************/

/**
 * Transmits the measurements currently buffered
 */
void lora_transmit_measurements();

/**
 * The callback for incomming promiscous packets
 * 
 * @param buffer the buffer which contains the data of the packet
 * @param type the type of packet
 */
void promisc_packet_cb(void *buffer, wifi_promiscuous_pkt_type_t type);

/**
  * Performs all the initialization code
 */
void setup();

/**
 * Switches the channels
 */
void loop();

#endif
#endif
