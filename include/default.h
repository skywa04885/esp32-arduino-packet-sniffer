/*
  Copyright Cybox 2020 - Written by Luke A.C.A. Rieff
*/

#ifndef _INCLUDE_DEFAULT_H
#define _INCLUDE_DEFAULT_H

#define COMPILE_AS_RECEIVER

#include <Arduino.h>
#include <LoRa.h>

#include <esp_wifi.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <esp_task_wdt.h>
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"

/*******************************
 * Board pin configuration
 ******************************/

#define SCK     5
#define MISO    19
#define MOSI    27
#define SS      18
#define RST     14
#define DI0     26
#define BAND    868E6

/*******************************
 * Pre-compile config
 ******************************/

#define GLOBAL_MEASUREMENT_BUFFER_SIZE 32
#define GLOBAL_CHANNEL_SWITCH_DELAY 10      /* In milliseconds */
#define GLOBAL_RECEIVER_BUFFER_SIZE 2048    /* Bytes */
#define GLOBAL_USART_BAUD 230400

/*******************************
 * Device information
 ******************************/

#ifndef PRODUCTION_FLASH
#define DEVICE_MAC  { 0x1,  0x2, 0x3, 0x4, 0x5, 0x6 }
#define GATEWAY_MAC { 0x12, 0x4, 0x2, 0x8, 0x7, 0x5 }
#endif

/*******************************
 * Types
 ******************************/

typedef struct __attribute__ (( packed )) {
  uint8_t mac[6];
} measurement_t;

#endif
