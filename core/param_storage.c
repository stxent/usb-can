/*
 * core/param_storage.c
 * Copyright (C) 2019 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <string.h>
#include <xcore/crc/crc32.h>
#include "helpers.h"
#include "param_storage.h"
/*----------------------------------------------------------------------------*/
void makeSerialNumber(struct SerialNumber *output, uint64_t input)
{
  inPlaceBinToHex4(output->value, (uint16_t)(input >> 48));
  inPlaceBinToHex4(output->value + 4, (uint16_t)(input >> 32));
  inPlaceBinToHex4(output->value + 8, (uint16_t)(input >> 16));
  inPlaceBinToHex4(output->value + 12, (uint16_t)input);
  output->value[16] = '\0';
}
/*----------------------------------------------------------------------------*/
void storageInit(struct ParamStorage *ps, struct Interface *memory,
    uint32_t offset)
{
  ps->memory = memory;
  ps->offset = offset;

  /* Set default values */
  ps->values.serial = (uint64_t)-1;
}
/*----------------------------------------------------------------------------*/
bool storageLoad(struct ParamStorage *ps)
{
  const uint32_t address = ps->offset;
  uint32_t crc;
  uint8_t buffer[sizeof(ps->values) + sizeof(crc)];

  if (ifSetParam(ps->memory, IF_POSITION, &address) != E_OK)
    return false;

  if (ifRead(ps->memory, &buffer, sizeof(buffer)) != sizeof(buffer))
    return false;

  memcpy(&crc, buffer + sizeof(ps->values), sizeof(crc));

  if (crc == crc32Update(0, buffer, sizeof(ps->values)))
  {
    memcpy(&ps->values, buffer, sizeof(ps->values));
    return true;
  }
  else
    return false;
}
/*----------------------------------------------------------------------------*/
bool storageSave(struct ParamStorage *ps)
{
  const uint32_t address = ps->offset;
  const uint32_t crc = crc32Update(0, &ps->values, sizeof(ps->values));
  uint8_t buffer[sizeof(ps->values) + sizeof(crc)];

  memcpy(buffer, &ps->values, sizeof(ps->values));
  memcpy(buffer + sizeof(ps->values), &crc, sizeof(crc));

  if (ifSetParam(ps->memory, IF_POSITION, &address) != E_OK)
    return false;

  if (ifWrite(ps->memory, buffer, sizeof(buffer)) != sizeof(buffer))
    return false;

  return true;
}
