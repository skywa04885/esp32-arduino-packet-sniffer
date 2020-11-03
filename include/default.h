#ifndef _INCLUDE_DEFAULT_H
#define _INCLUDE_DEFAULT_H

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

#define GLOBAL_MEASUREMENT_BUFFER_SIZE 24

#ifndef PRODUCTION_FLASH
#define DEVICE_MAC  { 0x1,  0x2, 0x3, 0x4, 0x5, 0x6 }
#define GATEWAY_MAC { 0x12, 0x4, 0x2, 0x8, 0x7, 0x5 }
#endif

#endif
