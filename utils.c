/**
 * @file utils.c
 * @author Phil Hilger (phil@peergum.com)
 * @brief 
 * @version 0.1
 * @date 2022-11-05
 * 
 * Copyright (c) 2022, PeerGum
 * 
 */

#include "utils.h"
// #include "time.h"
#include "esp_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
 * @brief returns milliseconds since boot
 *
 * @return unsigned long
 */
unsigned long millis() {
  struct timeval tv_now;
  gettimeofday(&tv_now, NULL);
  int64_t time_ms =
      (int64_t)tv_now.tv_sec * 1000L + (int64_t)tv_now.tv_usec / 1000L;
  return (unsigned long)time_ms;
}

unsigned long micros() {
  struct timeval tv_now;
  gettimeofday(&tv_now, NULL);
  int64_t time_ms =
      (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec / 1000000L;
  return (unsigned long)time_ms;
}

void delay(unsigned long ms) { vTaskDelay(pdMS_TO_TICKS(ms)); }

void delayMicroseconds(unsigned long us) { usleep(us); }

#ifdef __cplusplus
uint16_t makeWord(uint16_t w) { return w; }
uint16_t makeWord(uint8_t h, uint8_t l) { return (h << 8) | l; }
#endif