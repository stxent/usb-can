/*
 * core/settings.c
 * Copyright (C) 2024 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "settings_project.h"
#include <xcore/crc/crc8_maxim.h>
#include <xcore/crc/crc32_ieee.h>
#include <xcore/interface.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
/*----------------------------------------------------------------------------*/
#ifdef SETTINGS_CRC_32BIT
typedef uint32_t ChecksumType;

#  define CRC_FUNCTOR crc32IEEEUpdate
#  define CRC_INITIAL 0xFFFFFFFFUL
#else
typedef uint8_t ChecksumType;

#  define CRC_FUNCTOR crc8MaximUpdate
#  define CRC_INITIAL 0xFF
#endif
/*----------------------------------------------------------------------------*/
static void onMemoryEvent(void *);

extern const struct SettingsEntry ENTRY_MAP[];
/*----------------------------------------------------------------------------*/
bool settingsReadByIndex(const struct Settings *settings, uint16_t index,
    int32_t *value, int32_t *min, int32_t *max, char *name)
{
  if (index < ARRAY_SIZE(settings->overlay))
  {
    assert(strlen(ENTRY_MAP[index].name) <= CONFIG_NAME_LENGTH);

    if (min != NULL)
      *min = ENTRY_MAP[index].min;
    if (max != NULL)
      *max = ENTRY_MAP[index].max;
    if (name != NULL)
      strcpy(name, ENTRY_MAP[index].name);

    if (value != NULL)
      *value = settings->overlay[index];
    return true;
  }
  else
    return false;
}
/*----------------------------------------------------------------------------*/
bool settingsWriteByIndex(struct Settings *settings, uint16_t index,
    int32_t value)
{
  if (index < ARRAY_SIZE(settings->overlay))
  {
    if (value >= ENTRY_MAP[index].min && value <= ENTRY_MAP[index].max)
    {
      settings->overlay[index] = value;
      return true;
    }
  }

  return false;
}
/*----------------------------------------------------------------------------*/
#ifdef STRING_ENTRIES_COUNT
bool settingsReadStringByIndex(const struct Settings *settings, uint16_t index,
    char *value)
{
  if (index >= STR_SYSTEM_START)
  {
    /* Remap to system index */
    index -= STRING_SYSTEM_OFFSET;
  }

  if (index > ARRAY_SIZE(settings->strings))
    return false;

  strcpy(value, settings->strings[index].data);
  return true;
}
#endif
/*----------------------------------------------------------------------------*/
#ifdef STRING_ENTRIES_COUNT
bool settingsWriteStringByIndex(struct Settings *settings, uint16_t index,
    const char *value)
{
  const size_t count = strlen(value);

  if (index >= STR_SYSTEM_START)
  {
    /* Remap to system index */
    index -= STRING_SYSTEM_OFFSET;
  }

  if (count > sizeof(StringType) - 1)
    return false;
  if (index > ARRAY_SIZE(settings->strings))
    return false;

  memset(settings->strings[index].data, 0, sizeof(StringType));
  memcpy(settings->strings[index].data, value, count);
  return true;
}
#endif
/*----------------------------------------------------------------------------*/
bool settingsInit(struct SettingsContext *context, struct Interface *memory,
    uint32_t offset, void *data, size_t size)
{
  context->buffer = malloc(size + sizeof(ChecksumType));
  if (context->buffer == NULL)
    return false;

  context->memory = memory;
  context->offset = offset;
  context->data = data;
  context->size = size;

  context->callback = NULL;
  context->argument = NULL;
  context->direction = false;

  return true;
}
/*----------------------------------------------------------------------------*/
void settingsDeinit(struct SettingsContext *context)
{
  free(context->buffer);
}
/*----------------------------------------------------------------------------*/
void settingsLoad(struct SettingsContext *context,
    void (*callback)(void *, bool), void *argument)
{
  context->callback = callback;
  context->argument = argument;
  context->direction = false;

  ifSetParam(context->memory, IF_ZEROCOPY, NULL);
  ifSetCallback(context->memory, onMemoryEvent, context);
  ifSetParam(context->memory, IF_POSITION, &context->offset);
  ifRead(context->memory, context->buffer,
      context->size + sizeof(ChecksumType));
}
/*----------------------------------------------------------------------------*/
void settingsSave(struct SettingsContext *context,
    void (*callback)(void *, bool), void *argument)
{
  context->callback = callback;
  context->argument = argument;
  context->direction = true;

  const ChecksumType checksum = CRC_FUNCTOR(CRC_INITIAL, context->data,
      context->size);

  memcpy(context->buffer, context->data, context->size);
  memcpy(context->buffer + context->size, &checksum, sizeof(checksum));

  ifSetParam(context->memory, IF_ZEROCOPY, NULL);
  ifSetCallback(context->memory, onMemoryEvent, context);
  ifSetParam(context->memory, IF_POSITION, &context->offset);
  ifWrite(context->memory, context->buffer,
      context->size + sizeof(ChecksumType));
}
/*----------------------------------------------------------------------------*/
static void onMemoryEvent(void *argument)
{
  struct SettingsContext * const context = argument;
  const enum Result status = ifGetParam(context->memory, IF_STATUS, NULL);
  bool success = (status == E_OK);

  ifSetCallback(context->memory, NULL, NULL);

  if (status == E_OK && !context->direction)
  {
    const ChecksumType checksum = CRC_FUNCTOR(CRC_INITIAL, context->buffer,
        context->size);

    if (!memcmp(context->buffer + context->size, &checksum, sizeof(checksum)))
      memcpy(context->data, context->buffer, context->size);
    else
      success = false;
  }

  if (context->callback != NULL)
    context->callback(context->argument, success);
}
