/*
 * core/eeprom_24xx.h
 * Copyright (C) 2019 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef CORE_EEPROM_24XX_H_
#define CORE_EEPROM_24XX_H_
/*----------------------------------------------------------------------------*/
#include <xcore/interface.h>
#include <stdint.h>
/*----------------------------------------------------------------------------*/
extern const struct InterfaceClass * const Eeprom24xx;

struct Eeprom24xxConfig
{
  /** Mandatory: underlying I2C interface. */
  struct Interface *i2c;
  /** Mandatory: address space size. */
  uint32_t chipSize;
  /** Optional: baud rate of the interface. */
  uint32_t rate;
  /** Mandatory: bus address. */
  uint16_t address;
  /** Mandatory: page size. */
  uint16_t pageSize;
  /** Mandatory: block count. */
  uint8_t blocks;
};

struct Eeprom24xx
{
  struct Interface base;

  struct Interface *i2c;
  uint8_t *buffer;

  uint32_t chipSize;
  uint32_t position;
  uint32_t rate;

  uint16_t pageSize;
  uint16_t address;
  uint8_t width;
};
/*----------------------------------------------------------------------------*/
#endif /* CORE_EEPROM_24XX_H_ */
