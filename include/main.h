#ifndef _MAIN_H
#define _MAIN_H

#include "default.h"
#include "ieee80211.h"
#include "cbxpkt.h"

typedef struct {
  uint8_t mac[6];
} measurement_t;

void setup();
void loop();

#endif
