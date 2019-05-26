/*
 * board_shared.c
 * Copyright (C) 2019 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <halm/platform/nxp/lpc17xx/clocking.h>
#include "board_shared.h"
/*----------------------------------------------------------------------------*/
static const struct ExternalOscConfig extOscConfig = {
    .frequency = 12000000
};

static const struct PllConfig sysPllConfig = {
    .source = CLOCK_EXTERNAL,
    .divisor = 6,
    .multiplier = 30
};

static const struct PllConfig usbPllConfig = {
    .source = CLOCK_EXTERNAL,
    .divisor = 4,
    .multiplier = 16
};

static const struct GenericClockConfig mainClockConfig = {
    .source = CLOCK_PLL
};

static const struct GenericClockConfig usbClockConfig = {
    .source = CLOCK_USB_PLL
};
/*----------------------------------------------------------------------------*/
void boardSetupClock(void)
{
  enum Result res __attribute__((unused));

  res = clockEnable(ExternalOsc, &extOscConfig);
  assert(res == E_OK);
  while (!clockReady(ExternalOsc));

  res = clockEnable(SystemPll, &sysPllConfig);
  assert(res == E_OK);
  while (!clockReady(SystemPll));

  res = clockEnable(UsbPll, &usbPllConfig);
  assert(res == E_OK);
  while (!clockReady(UsbPll));

  res = clockEnable(MainClock, &mainClockConfig);
  assert(res == E_OK);

  res = clockEnable(UsbClock, &usbClockConfig);
  assert(res == E_OK);
  while (!clockReady(UsbClock));
}
