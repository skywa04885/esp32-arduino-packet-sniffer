/*
  Copyright Cybox 2020 - Written by Luke A.C.A. Rieff
*/

#ifndef _CBXPKT_H
#define _CBXPKT_H

#include "default.h"
#include "ieee80211.h"

/*******************************
 * Types
 ******************************/

typedef struct __attribute__ (( packed )) {
  unsigned encrypted : 1;       /* If the packet has been encrypted */
  unsigned relayed : 1;         /* If the packet has been relayed */
  unsigned chained : 1;         /* If the packet is chained */
  unsigned reserved : 5;
} cbx_pkt_flags_t;

typedef struct __attribute__ (( packed )) {
  char label[4];                /* The packet label, mostly 'CBXL' */
  uint8_t sender[6];            /* The sender address */
  uint8_t receiver[6];          /* The receiver address */
  uint8_t chain_no;             /* The number of the current chain */
  cbx_pkt_flags_t flags;        /* The packet flags */
} cbx_pkt_hdr_t;

typedef struct __attribute__ (( packed )) {
  uint32_t unique_id;           /* The unique identifier of the packet */
  uint8_t size;                 /* The size of the payload */
  uint8_t *payload;             /* The body of the packet */
} cbx_pkt_body_t;

typedef struct __attribute__ (( packed )) {
  cbx_pkt_hdr_t hdr;            /* The header of the packet */
  cbx_pkt_body_t body;          /* The body of the packet */
} cbx_pkt_t;

/*******************************
 * Function prototypes
 ******************************/

/**
 * Transmits an packet over the LoRa antenna
 * 
 * @param pkt the packet to be transmitted
 */
void cbx_pkt_transmit(const cbx_pkt_t *pkt);

/**
 * Logs packet details over the USART line
 * 
 * @param pkt the packet to be logged
 */
void cbx_pkt_log(const cbx_pkt_t *pkt);

#endif
