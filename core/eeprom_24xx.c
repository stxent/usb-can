/*
 * core/eeprom_24xx.c
 * Copyright (C) 2019 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "eeprom_24xx.h"
#include <halm/delay.h> // XXX
#include <xcore/accel.h>
#include <xcore/bits.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
/*----------------------------------------------------------------------------*/
#define WRITE_CYCLE_TIME 5
/*----------------------------------------------------------------------------*/
static inline size_t calcAddressWidth(const struct Eeprom24xx *);
static void fillDataAddress(struct Eeprom24xx *, uint32_t);
static uint16_t makeSlaveAddress(const struct Eeprom24xx *, uint32_t);
/*----------------------------------------------------------------------------*/
static enum Result memoryInit(void *, const void *);
static void memoryDeinit(void *);
static enum Result memoryGetParam(void *, enum IfParameter, void *);
static enum Result memorySetParam(void *, enum IfParameter, const void *);
static size_t memoryRead(void *, void *, size_t);
static size_t memoryWrite(void *, const void *, size_t);
/*----------------------------------------------------------------------------*/
const struct InterfaceClass * const Eeprom24xx = &(const struct InterfaceClass){
    .size = sizeof(struct Eeprom24xx),
    .init = memoryInit,
    .deinit = memoryDeinit,

    .setCallback = 0,
    .getParam = memoryGetParam,
    .setParam = memorySetParam,
    .read = memoryRead,
    .write = memoryWrite
};
/*----------------------------------------------------------------------------*/
static inline size_t calcAddressWidth(const struct Eeprom24xx *memory)
{
  return (memory->width + 7) / 8;
}
/*----------------------------------------------------------------------------*/
static void fillDataAddress(struct Eeprom24xx *memory, uint32_t globalAddress)
{
  const uint32_t address = globalAddress & MASK(memory->width);
  const size_t width = calcAddressWidth(memory) - 1;

  for (size_t i = 0; i <= width; ++i)
    memory->buffer[i] = (uint8_t)(address >> ((width - i) * 8));
}
/*----------------------------------------------------------------------------*/
static uint16_t makeSlaveAddress(const struct Eeprom24xx *memory,
    uint32_t globalAddress)
{
  const uint16_t block = globalAddress >> memory->width;
  return memory->address | block;
}
/*----------------------------------------------------------------------------*/
static enum Result memoryInit(void *object, const void *configBase)
{
  const struct Eeprom24xxConfig * const config = configBase;
  assert(config);

  if (!config->blocks || !config->chipSize || !config->pageSize)
    return E_VALUE;

  /* Determine if the size is a power of 2 */
  if ((config->chipSize & (config->chipSize - 1)) != 0)
    return E_VALUE;

  struct Eeprom24xx * const memory = object;

  memory->i2c = config->i2c;
  memory->address = config->address;
  memory->rate = config->rate;

  memory->chipSize = config->chipSize;
  memory->pageSize = config->pageSize;
  memory->position = 0;

  const uint32_t addressWidth =
      31 - countLeadingZeros32(config->chipSize / config->blocks);

  memory->buffer = malloc(memory->pageSize + (addressWidth + 7) / 8);
  memory->width = addressWidth;

  return memory->buffer ? E_OK : E_MEMORY;
}
/*----------------------------------------------------------------------------*/
static void memoryDeinit(void *object)
{
  struct Eeprom24xx * const memory = object;
  free(memory->buffer);
}
/*----------------------------------------------------------------------------*/
static enum Result memoryGetParam(void *object, enum IfParameter parameter,
    void *data)
{
  struct Eeprom24xx * const memory = object;

  switch (parameter)
  {
    case IF_POSITION:
      *(uint32_t *)data = memory->position;
      return E_OK;

    case IF_SIZE:
      *(uint32_t *)data = memory->chipSize;
      return E_OK;

    default:
      return E_INVALID;
  }
}
/*----------------------------------------------------------------------------*/
static enum Result memorySetParam(void *object, enum IfParameter parameter,
    const void *data)
{
  struct Eeprom24xx * const memory = object;

  switch (parameter)
  {
    case IF_POSITION:
    {
      const uint32_t position = *(const uint32_t *)data;

      if (position < memory->chipSize)
      {
        memory->position = position;
        return E_OK;
      }
      else
        return E_ADDRESS;
    }

    default:
      return E_INVALID;
  }
}
/*----------------------------------------------------------------------------*/
static size_t memoryRead(void *object, void *buffer, size_t length)
{
  struct Eeprom24xx * const memory = object;
  const uint32_t end = memory->position + length;
  uint8_t *position = buffer;

  if (end > memory->chipSize)
    return 0;

  /* Lock the interface */
  ifSetParam(memory->i2c, IF_ACQUIRE, 0);

  /* Set rate */
  if (memory->rate && ifSetParam(memory->i2c, IF_RATE, &memory->rate) != E_OK)
    goto read_end;

  const size_t addressWidth = calcAddressWidth(memory);
  uint32_t start = memory->position;

  /* Read the memory chunk by chunk */
  while (start < end)
  {
    const uint32_t nextAlignedPosition =
        ((start / memory->pageSize) + 1) * memory->pageSize;
    const size_t chunkLength = MIN(nextAlignedPosition - start, end - start); // XXX
    const uint16_t slaveAddress = makeSlaveAddress(memory, start);

    fillDataAddress(memory, start);

    if (ifSetParam(memory->i2c, IF_ADDRESS, &slaveAddress) != E_OK)
      goto read_end;
    if (ifWrite(memory->i2c, memory->buffer, addressWidth) != addressWidth)
      goto read_end;
    if (ifRead(memory->i2c, memory->buffer, chunkLength) != chunkLength)
      goto read_end;

    memcpy(position, memory->buffer, chunkLength);

    position += chunkLength;
    start += chunkLength;
  }

  memory->position = end;

read_end:
  /* Unlock the interface */
  ifSetParam(memory->i2c, IF_RELEASE, 0);
  return (size_t)(position - (uint8_t *)buffer);
}
/*----------------------------------------------------------------------------*/
static size_t memoryWrite(void *object, const void *buffer, size_t length)
{
  struct Eeprom24xx * const memory = object;
  const uint32_t end = memory->position + length;
  const uint8_t *position = buffer;

  if (end > memory->chipSize)
    return 0;

  /* Lock the interface */
  ifSetParam(memory->i2c, IF_ACQUIRE, 0);

  /* Set rate */
  if (memory->rate && ifSetParam(memory->i2c, IF_RATE, &memory->rate) != E_OK)
    goto write_end;

  const size_t addressWidth = calcAddressWidth(memory);
  uint32_t start = memory->position;

  /* Read the memory chunk by chunk */
  while (start < end)
  {
    const uint32_t nextAlignedPosition =
        ((start / memory->pageSize) + 1) * memory->pageSize;
    const size_t chunkLength = MIN(nextAlignedPosition - start, end - start); // XXX
    const size_t transferLength = addressWidth + chunkLength;
    const uint16_t slaveAddress = makeSlaveAddress(memory, start);

    fillDataAddress(memory, start);
    memcpy(memory->buffer + addressWidth, position, chunkLength);

    if (ifSetParam(memory->i2c, IF_ADDRESS, &slaveAddress) != E_OK)
      goto write_end;
    if (ifWrite(memory->i2c, memory->buffer, transferLength) != transferLength)
      goto write_end;
    mdelay(WRITE_CYCLE_TIME);

    position += chunkLength;
    start += chunkLength;
  }

  memory->position = end;

write_end:
  /* Unlock the interface */
  ifSetParam(memory->i2c, IF_RELEASE, 0);
  return (size_t)(position - (const uint8_t *)buffer);
}
