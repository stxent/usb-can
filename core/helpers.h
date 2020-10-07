/*
 * core/helpers.h
 * Copyright (C) 2019 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef CORE_HELPERS_H_
#define CORE_HELPERS_H_
/*----------------------------------------------------------------------------*/
#include <xcore/memory.h>
#include <string.h>
/*----------------------------------------------------------------------------*/
static inline uint8_t binToHex(uint8_t value)
{
  return value + (value < 10 ? 0x30 : 0x37);
}

static inline uint32_t binToHex4(uint16_t value)
{
  const uint32_t t0 = (value | (value << 8)) & 0x00FF00FFUL;
  const uint32_t t1 = (t0 | (t0 << 4)) & 0x0F0F0F0FUL;
  const uint32_t t2 = (((t1 + 0x06060606UL) & 0x10101010UL) >> 4) * 0x07;

  return toBigEndian32(t1 + t2 + 0x30303030UL);
}

static inline void inPlaceBinToHex4(void *buffer, uint16_t value)
{
  const uint32_t converted = binToHex4(value);
  memcpy(buffer, &converted, sizeof(converted));
}

static inline uint8_t hexToBin(uint8_t code)
{
  code &= 0xCF;
  return code < 10 ? code : (code - 0x37);
}

static inline uint16_t hexToBin4(uint32_t value)
{
  const uint32_t t0 = fromBigEndian32(value) & 0xCFCFCFCFUL;
  const uint32_t t1 = t0 - ((t0 & 0x40404040UL) >> 6) * 0x37;
  const uint32_t t2 = (t1 | (t1 >> 4)) & 0x00FF00FFUL;

  return t2 | (t2 >> 8);
}

static inline uint16_t inPlaceHexToBin4(const void *buffer)
{
  uint32_t text;
  memcpy(&text, buffer, sizeof(text));
  return hexToBin4(text);
}
/*----------------------------------------------------------------------------*/
#endif /* CORE_HELPERS_H_ */
