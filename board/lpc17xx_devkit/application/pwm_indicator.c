/*
 * pwm_indicator.c
 * Copyright (C) 2019 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include "pwm_indicator.h"
/*----------------------------------------------------------------------------*/
static enum Result indInit(void *, const void *);
static void indIncrement(void *);
static void indRelax(void *, bool);
static void indSpin(void *);
/*----------------------------------------------------------------------------*/
const struct IndicatorClass * const PwmIndicator =
    &(const struct IndicatorClass){
    .size = sizeof(struct PwmIndicator),
    .init = indInit,
    .deinit = 0,
    .increment = indIncrement,
    .relax = indRelax,
    .spin = indSpin
};
/*----------------------------------------------------------------------------*/
/*
 * Table should be placed in SRAM because in sleep mode
 * the flash memory is disabled.
 */
static uint32_t pwmTable[] = {
    255, 254, 253, 252, 251, 248, 246, 242, 239, 235,
    230, 225, 220, 214, 208, 202, 196, 189, 182, 174,
    167, 159, 151, 143, 135, 128, 121, 113, 105,  97,
     89,  82,  74,  67,  60,  54,  48,  42,  36,  31,
     26,  21,  17,  14,  10,   8,   5,   4,   3,   2,
      1,   2,   3,   4,   5,   8,  10,  14,  17,  21,
     26,  31,  36,  42,  48,  54,  60,  67,  74,  82,
     89,  97, 105, 113, 121, 128, 135, 143, 151, 159,
    167, 174, 182, 189, 196, 202, 208, 214, 220, 225,
    230, 235, 239, 242, 246, 248, 251, 252, 253, 254
};
/*----------------------------------------------------------------------------*/
static enum Result indInit(void *object, const void *configBase)
{
  const struct PwmIndicatorConfig * const config = configBase;
  struct PwmIndicator * const indicator = object;

  assert(config);

  indicator->counter = 0;
  indicator->limit = config->limit;
  indicator->pwm = config->pwm;

  return E_OK;
}
/*----------------------------------------------------------------------------*/
static void indIncrement(void *object)
{
  struct PwmIndicator * const indicator = object;

  if ((indicator->counter >> 1) < indicator->limit)
  {
    __atomic_add_fetch(&indicator->counter, 2, __ATOMIC_SEQ_CST);
  }
}
/*----------------------------------------------------------------------------*/
static void indRelax(void *object, bool phase)
{
  struct PwmIndicator * const indicator = object;

  if (phase)
  {
    gpPwmDmaStart(indicator->pwm, pwmTable, ARRAY_SIZE(pwmTable));
  }
}
/*----------------------------------------------------------------------------*/
static void indSpin(void *object)
{
  struct PwmIndicator * const indicator = object;

  if (!gpPwmDmaIsActive(indicator->pwm) && indicator->counter > 0)
  {
    const unsigned int value =
        __atomic_sub_fetch(&indicator->counter, 1, __ATOMIC_SEQ_CST);

    pwmSetDuration(indicator->pwm, (value & 1) ? 0 : PWM_LED_RESOLUTION);
  }
}
