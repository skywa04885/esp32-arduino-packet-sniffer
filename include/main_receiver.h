/*
  Copyright Cybox 2020 - Written by Luke A.C.A. Rieff
*/

#ifndef _MAIN_RECEIVER_H
#define _MAIN_RECEIVER_H

#include "default.h"
#include "cbxpkt.h"
#include "server_connection.h"

#ifdef COMPILE_AS_RECEIVER

/*******************************
 * Function prototypes
 ******************************/

/**
 * Handles ESP events
 * 
 * @param ctx the context or something
 * @param event the event that occured
 */
esp_err_t event_handler(void *ctx, system_event_t *event);

/**
  * Performs all the initialization code
 */
void setup();

/**
 * Performs the receiving
 */
void loop();

#endif
#endif