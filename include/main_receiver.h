/*
  Copyright Cybox 2020 - Written by Luke A.C.A. Rieff
*/

#ifndef _MAIN_RECEIVER_H
#define _MAIN_RECEIVER_H

#include "default.h"
#include "cbxpkt.h"

#ifdef COMPILE_AS_RECEIVER

/*******************************
 * Function prototypes
 ******************************/

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