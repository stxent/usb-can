/*
 * core/settings.h
 * Copyright (C) 2024 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef CORE_SETTINGS_H_
#define CORE_SETTINGS_H_
/*----------------------------------------------------------------------------*/
#include <xcore/helpers.h>
#include <stdint.h>
#include <stddef.h>
/*----------------------------------------------------------------------------*/
#define CONFIG_NAME_LENGTH    32
#define STRING_CONFIG_LENGTH  24

typedef struct
{
  char data[STRING_CONFIG_LENGTH];
} StringType;
/*----------------------------------------------------------------------------*/
struct Interface;
struct Settings;

struct SettingsEntry
{
  const char *name;
  int32_t min;
  int32_t max;
};

struct SettingsContext
{
  struct Interface *memory;
  uint32_t offset;

  void *data;
  size_t size;

  void (*callback)(void *, bool);
  void *argument;

  uint8_t *buffer;
  bool direction; /* Read - false, write - true */
};
/*----------------------------------------------------------------------------*/
BEGIN_DECLS

bool settingsReadByIndex(const struct Settings *, uint16_t, int32_t *,
    int32_t *, int32_t *, char *);
bool settingsWriteByIndex(struct Settings *, uint16_t, int32_t);
bool settingsReadStringByIndex(const struct Settings *, uint16_t, char *);
bool settingsWriteStringByIndex(struct Settings *, uint16_t, const char *);

bool settingsInit(struct SettingsContext *, struct Interface *, uint32_t,
    void *, size_t);
void settingsDeinit(struct SettingsContext *);
void settingsLoad(struct SettingsContext *, void (*)(void *, bool), void *);
void settingsSave(struct SettingsContext *, void (*)(void *, bool), void *);

END_DECLS
/*----------------------------------------------------------------------------*/
#endif /* CORE_SETTINGS_H_ */
