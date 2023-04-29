/*
 * core/led_indicator.c
 * Copyright (C) 2019 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "led_indicator.h"
#include <xcore/atomic.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
static enum Result indInit(void *, const void *);
static void indIncrement(void *);
static void indRelax(void *, bool);
static void indSet(void *, unsigned int);
static void indSpin(void *);
/*----------------------------------------------------------------------------*/
const struct IndicatorClass * const LedIndicator =
    &(const struct IndicatorClass){
    .size = sizeof(struct LedIndicator),
    .init = indInit,
    .deinit = NULL,
    .increment = indIncrement,
    .relax = indRelax,
    .set = indSet,
    .spin = indSpin
};
/*----------------------------------------------------------------------------*/
static enum Result indInit(void *object, const void *configBase)
{
  const struct LedIndicatorConfig * const config = configBase;
  assert(config != NULL);

  struct LedIndicator * const indicator = object;

  indicator->counter = 0;
  indicator->limit = config->limit;
  indicator->inversion = config->inversion;

  indicator->led = pinInit(config->pin);
  pinOutput(indicator->led, indicator->inversion);

  return E_OK;
}
/*----------------------------------------------------------------------------*/
static void indIncrement(void *object)
{
  struct LedIndicator * const indicator = object;

  if ((indicator->counter >> 1) < indicator->limit)
  {
    atomicFetchAddU(&indicator->counter, 2);
  }
}
/*----------------------------------------------------------------------------*/
static void indRelax(void *object, bool phase)
{
  struct LedIndicator * const indicator = object;
  pinWrite(indicator->led, phase ^ indicator->inversion);
}
/*----------------------------------------------------------------------------*/
static void indSet(void *object, unsigned int value)
{
  struct LedIndicator * const indicator = object;
  indicator->counter = value;
}
/*----------------------------------------------------------------------------*/
static void indSpin(void *object)
{
  struct LedIndicator * const indicator = object;

  if (indicator->counter > 0)
  {
    const unsigned int value = atomicFetchSubU(&indicator->counter, 1);
    const bool state = (value & 1) == 0;

    pinWrite(indicator->led, state ^ indicator->inversion);
  }
}
