/*
 * board/lpc17xx_devkit/pwm_indicator.h
 * Copyright (C) 2019 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef BOARD_LPC17XX_DEVKIT_PWM_INDICATOR_H_
#define BOARD_LPC17XX_DEVKIT_PWM_INDICATOR_H_
/*----------------------------------------------------------------------------*/
#include "gppwm_dma.h"
#include "indicator.h"
/*----------------------------------------------------------------------------*/
#define PWM_LED_FREQUENCY   1000
#define PWM_LED_RESOLUTION  255
#define PWM_LED_TABLE_SIZE  100

#define PWM_LED_LATCH_RATE  200
#define PWM_LED_UPDATE_RATE 100
/*----------------------------------------------------------------------------*/
extern const struct IndicatorClass * const PwmIndicator;

struct PwmIndicatorConfig
{
  unsigned int limit;
  struct GpPwmDma *pwm;
};

struct PwmIndicator
{
  struct Indicator base;

  struct GpPwmDma *pwm;

  unsigned int counter;
  unsigned int limit;
};
/*----------------------------------------------------------------------------*/
#endif /* BOARD_LPC17XX_DEVKIT_PWM_INDICATOR_H_ */
