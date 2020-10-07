/*
 * core/led_indicator.h
 * Copyright (C) 2019 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef CORE_LED_INDICATOR_H_
#define CORE_LED_INDICATOR_H_
/*----------------------------------------------------------------------------*/
#include "indicator.h"
#include <halm/pin.h>
/*----------------------------------------------------------------------------*/
extern const struct IndicatorClass * const LedIndicator;

struct LedIndicatorConfig
{
  unsigned int limit;
  PinNumber pin;
  bool inversion;
};

struct LedIndicator
{
  struct Indicator base;

  struct Pin led;

  unsigned int counter;
  unsigned int limit;
  bool inversion;
};
/*----------------------------------------------------------------------------*/
#endif /* CORE_LED_INDICATOR_H_ */
