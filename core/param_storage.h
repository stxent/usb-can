/*
 * core/param_storage.h
 * Copyright (C) 2019 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef CORE_PARAM_STORAGE_H_
#define CORE_PARAM_STORAGE_H_
/*----------------------------------------------------------------------------*/
#include <xcore/interface.h>
#include <stdbool.h>
#include <stdint.h>
/*----------------------------------------------------------------------------*/
struct SerialNumber
{
  char value[sizeof(uint64_t) * 2 + 1];
};

struct ParamStorage
{
  struct Interface *memory;
  uint32_t offset;

  struct {
    uint64_t serial;
  } __attribute__((packed)) values;
};
/*----------------------------------------------------------------------------*/
static inline bool isSerialNumberValid(uint64_t number)
{
  return number != 0 && number != (uint64_t)-1;
}
/*----------------------------------------------------------------------------*/
BEGIN_DECLS

void makeSerialNumber(struct SerialNumber *, uint64_t);
void storageInit(struct ParamStorage *, struct Interface *, uint32_t);
bool storageLoad(struct ParamStorage *);
bool storageSave(struct ParamStorage *);

END_DECLS
/*----------------------------------------------------------------------------*/
#endif /* CORE_PARAM_STORAGE_H_ */
