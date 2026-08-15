/*
 * core/settings_project.h
 * Copyright (C) 2026 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef CORE_SETTINGS_PROJECT_H_
#define CORE_SETTINGS_PROJECT_H_
/*----------------------------------------------------------------------------*/
#include "can_proxy.h"
#include "settings.h"
/*----------------------------------------------------------------------------*/
#define SETTINGS_CRC_32BIT

enum StringIndex
{
  STR_SYSTEM_START  = 0xF000,

  STR_VENDOR_NAME   = 0xF000, /* Same as STR_SYSTEM_START */
  STR_DEVICE_NAME   = 0xF001,

  STR_SYSTEM_END
};

#define INITIAL_MODE_MIN      -1
#define INITIAL_MODE_MAX      8
#define SERIAL_NUMBER_LENGTH  (sizeof(int32_t) * 2 + 1)

#define STRING_SYSTEM_OFFSET  (STR_SYSTEM_START - 0)
#define STRING_ENTRIES_COUNT  (STR_SYSTEM_END - STRING_SYSTEM_OFFSET)

struct [[gnu::packed]] Settings
{
  union
  {
    struct
    {
      int32_t serial;
      int32_t initial[SLCAN_PORT_END];
    };

    int32_t overlay[1 + SLCAN_PORT_END];
  };

  StringType strings[STRING_ENTRIES_COUNT];
};
/*----------------------------------------------------------------------------*/
BEGIN_DECLS

static inline bool isSerialNumberValid(int32_t number)
{
  return number != 0 && number != -1;
}

void makeSerialNumber(char *, int32_t);
void settingsLoadDefault(struct Settings *);

END_DECLS
/*----------------------------------------------------------------------------*/
#endif /* CORE_SETTINGS_PROJECT_H_ */
