/*
  Copyright Cybox 2020 - Written by Luke A.C.A. Rieff
*/

#ifndef _INCLUDE_DEFAULT_H
#define _INCLUDE_DEFAULT_H

#define COMPILE_AS_RECEIVER
#define GLOBAL_DEBUG

#include <Arduino.h>
#include <lib/LoRa.h>

#include <stdio.h>
#include <esp_wifi.h>
#include <esp_log.h>
#include <chrono>
#include <esp_tls.h>
#include <nvs_flash.h>
#include <esp_task_wdt.h>
#include <esp_http_client.h>
#include <esp_event_loop.h>
#include <esp_event.h>
#include <esp_timer.h>
#include <cJSON.h>

#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"

/*******************************
 * Board pin configuration
 ******************************/

#define SCK             5
#define MISO            19
#define MOSI            27
#define SS              18
#define RST             14
#define DI0             26

/*******************************
 * Pre-compile config
 ******************************/

#define GLOBAL_MEASUREMENT_BUFFER_SIZE 32
#define GLOBAL_CHANNEL_SWITCH_DELAY 10      /* In milliseconds */
#define GLOBAL_RECEIVER_BUFFER_SIZE 2048    /* Bytes */
#define GLOBAL_USART_BAUD 230400
#define BAND    868E6

/*******************************
 * Device information
 ******************************/

#ifndef PRODUCTION_FLASH
#define DEVICE_MAC  { 0x1,  0x2, 0x3, 0x4, 0x5, 0x6 }
#define GATEWAY_MAC { 0x12, 0x4, 0x2, 0x8, 0x7, 0x5 }
#define GLOBAL_WIFI_SSID "VRV951769FD81"
#define GLOBAL_WIFI_PASS "3mPohmrJ5GY@"
#define GLOBAL_API_KEY "8a3d6b-efcdc1-6de"
#define GLOBAL_SERVER_IP "192.168.2.11"
#define GLOBAL_SERVER_PORT 8801
#endif

/*******************************
 * Macros
 ******************************/

#ifdef GLOBAL_DEBUG
#define DEBUG_ONLY(A) A
#else
#define DEBUG_ONLY(A)
#endif

/*******************************
 * Types
 ******************************/

typedef struct __attribute__ (( packed )) {
  uint8_t mac[6];
} measurement_t;

#endif
