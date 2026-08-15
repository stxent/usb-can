/*
 * core/settings_project.c
 * Copyright (C) 2026 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "helpers.h"
#include "settings_project.h"
/*----------------------------------------------------------------------------*/
const struct SettingsEntry ENTRY_MAP[] = {
    {"serial", INT32_MIN, INT32_MAX},
    {"initial0", INITIAL_MODE_MIN, INITIAL_MODE_MAX},
    {"initial1", INITIAL_MODE_MIN, INITIAL_MODE_MAX},
    {"initial2", INITIAL_MODE_MIN, INITIAL_MODE_MAX}
};

static_assert(ARRAY_SIZE(ENTRY_MAP) ==
    ARRAY_SIZE(((struct Settings *)NULL)->overlay), "Incorrect overlay");
/*----------------------------------------------------------------------------*/
void makeSerialNumber(char *output, int32_t input)
{
  inPlaceBinToHex4(output, (uint32_t)input >> 16);
  inPlaceBinToHex4(output + 4, (uint32_t)input);
  output[8] = '\0';
}
/*----------------------------------------------------------------------------*/
void settingsLoadDefault(struct Settings *settings)
{
  settings->serial = 0;
  for (size_t i = 0; i < ARRAY_SIZE(settings->initial); ++i)
    settings->initial[i] = -1;
}
