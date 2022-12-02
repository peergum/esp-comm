/**
 * @file utils.h
 * @author Phil Hilger (phil@peergum.com)
 * @brief 
 * @version 0.1
 * @date 2022-11-05
 * 
 * Copyright (c) 2022, PeerGum
 * 
 */

#ifndef __UTILS_H__
#define __UTILS_H__

#define lowByte(w) ((uint8_t)((w)&0xff))
#define highByte(w) ((uint8_t)((w) >> 8))

#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitToggle(value, bit) ((value) ^= (1UL << (bit)))
#define bitWrite(value, bit, bitvalue) \
  ((bitvalue) ? bitSet((value), (bit)) : bitClear((value), (bit)))

#ifndef bit
#define bit(b) (1UL << (b))
#endif

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
unsigned long millis(void);
unsigned long micros(void);
void delay(unsigned long);
void delayMicroseconds(unsigned int us);
}
#endif

#ifdef __cplusplus
template <class T, class L>
auto min(const T& a, const L& b) -> decltype((b < a) ? b : a) {
  return (b < a) ? b : a;
}

template <class T, class L>
auto max(const T& a, const L& b) -> decltype((b < a) ? b : a) {
  return (a < b) ? b : a;
}
#else
#ifndef min
#define min(a, b)           \
  ({                        \
    __typeof__(a) _a = (a); \
    __typeof__(b) _b = (b); \
    _a < _b ? _a : _b;      \
  })
#endif
#ifndef max
#define max(a, b)           \
  ({                        \
    __typeof__(a) _a = (a); \
    __typeof__(b) _b = (b); \
    _a > _b ? _a : _b;      \
  })
#endif
#endif

#ifdef __cplusplus

/* C++ prototypes */
uint16_t makeWord(uint16_t w);
uint16_t makeWord(uint8_t h, uint8_t l);

#define word(...) makeWord(__VA_ARGS__)

#endif

#endif // __UTILS_H__