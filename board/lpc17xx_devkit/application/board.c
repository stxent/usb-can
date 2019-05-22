/*
 * board.c
 * Copyright (C) 2019 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <halm/core/cortex/systick.h>
#include <halm/pin.h>
#include <halm/platform/nxp/can.h>
#include <halm/platform/nxp/gptimer.h>
#include <halm/platform/nxp/serial.h>
#include <halm/platform/nxp/usb_device.h>
#include <halm/platform/nxp/lpc17xx/clocking.h>
#include <halm/usb/cdc_acm.h>
#include <halm/usb/composite_device.h>
#include "board.h"
#include "gppwm_dma.h"
#include "led_indicator.h"
#include "pwm_indicator.h"
/*----------------------------------------------------------------------------*/
#define EVENT_RATE 50
#define MAX_BLINKS 16
/*----------------------------------------------------------------------------*/
#define PORT0_LED PIN(2, 2)
#define PORT1_LED PIN(2, 0)
#define ERROR_LED PIN(2, 1)
/*----------------------------------------------------------------------------*/
static const struct CanConfig can1Config = {
    .rate = 10000,
    .rxBuffers = 16,
    .txBuffers = 16,
    .rx = PIN(0, 0),
    .tx = PIN(0, 1),
    .priority = 3,
    .channel = 0
};

static const struct CanConfig can2Config = {
    .rate = 10000,
    .rxBuffers = 16,
    .txBuffers = 16,
    .rx = PIN(2, 7),
    .tx = PIN(2, 8),
    .priority = 3,
    .channel = 1
};

static const struct GpTimerConfig pwmTimerConfig = {
    .frequency = 1000000,
    .channel = 3,
    .event = 3
};

static const struct GpTimerConfig pwmUnitTimerConfig = {
    .frequency = 1000000,
    .channel = 2,
    .event = 3
};

static const struct GpTimerConfig chronoTimerConfig = {
    .frequency = 1000000,
    .channel = 1
};

static const struct GpPwmDmaUnitConfig pwmUnitConfig = {
    .frequency = PWM_LED_FREQUENCY,
    .resolution = PWM_LED_RESOLUTION,
    .channel = 0,
    .dma = 7,
    .event = GPDMA_MAT2_0
};

static const struct UsbDeviceConfig usbConfig = {
    .dm = PIN(0, 30),
    .dp = PIN(0, 29),
    .connect = PIN(2, 9),
    .vbus = PIN(1, 30),
    .vid = 0x2404,
    .pid = 0x03EB,
    .priority = 1,
    .channel = 0
};

static const struct LedIndicatorConfig errorLedConfig = {
    .pin = ERROR_LED,
    .limit = MAX_BLINKS,
    .inversion = false
};

//// XXX
//static const struct LedIndicatorConfig port0LedConfig = {
//    .pin = PORT0_LED,
//    .limit = MAX_BLINKS,
//    .inversion = true
//};
//
//// XXX
//static const struct LedIndicatorConfig port1LedConfig = {
//    .pin = PORT1_LED,
//    .limit = MAX_BLINKS,
//    .inversion = true
//};
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
/*----------------------------------------------------------------------------*/
void boardSetup(struct Board *board)
{
  board->port0.can = init(Can, &can1Config);
  assert(board->port0.can);
  board->port1.can = init(Can, &can2Config);
  assert(board->port1.can);

  board->chronoTimer = init(GpTimer, &chronoTimerConfig);
  assert(board->chronoTimer);
  timerEnable(board->chronoTimer);

  board->eventTimer = init(SysTickTimer, 0);
  assert(board->eventTimer);
  timerSetOverflow(board->eventTimer,
      timerGetFrequency(board->eventTimer) / EVENT_RATE);

  board->pwmTimer = init(GpTimer, &pwmTimerConfig);
  assert(board->pwmTimer);
  timerSetOverflow(board->pwmTimer,
      pwmTimerConfig.frequency / PWM_LED_UPDATE_RATE);

  board->pwmUnitTimer = init(GpTimer, &pwmUnitTimerConfig);
  assert(board->pwmUnitTimer);
  timerSetOverflow(board->pwmUnitTimer,
      pwmTimerConfig.frequency / PWM_LED_LATCH_RATE);
  timerEnable(board->pwmUnitTimer);

  board->pwmUnit = init(GpPwmDmaUnit, &pwmUnitConfig);
  assert(board->pwmUnit);
  board->pwm0 = gpPwmDmaCreate(board->pwmUnit, PORT0_LED, 0, GPDMA_MAT3_0);
  assert(board->pwm0);
  pwmSetDuration(board->pwm0, PWM_LED_RESOLUTION);
  pwmEnable(board->pwm0);
  board->pwm1 = gpPwmDmaCreate(board->pwmUnit, PORT1_LED, 1, GPDMA_MAT3_1);
  assert(board->pwm1);
  pwmSetDuration(board->pwm1, PWM_LED_RESOLUTION);
  pwmEnable(board->pwm1);
//  board->port0.status = init(LedIndicator, &port0LedConfig);
//  assert(board->port0.status);
//  board->port1.status = init(LedIndicator, &port1LedConfig);
//  assert(board->port1.status);
  const struct PwmIndicatorConfig port0PwmConfig = {
      .limit = MAX_BLINKS,
      .pwm = board->pwm0
  };
  board->port0.status = init(PwmIndicator, &port0PwmConfig);
  assert(board->port0.status);
  const struct PwmIndicatorConfig port1PwmConfig = {
      .limit = MAX_BLINKS,
      .pwm = board->pwm1
  };
  board->port1.status = init(PwmIndicator, &port1PwmConfig);
  assert(board->port1.status);

//  activityIndicatorInit(&board->port0.indicator, board->port0.indicator.pwm,
//      MAX_BLINKS);
//  activityIndicatorInit(&board->port1.indicator, board->port1.indicator.pwm,
//      MAX_BLINKS);
//  pwmSetDuration(board->port0.indicator.pwm, PWM_LED_RESOLUTION);
//  pwmEnable(board->port0.indicator.pwm);
//  pwmSetDuration(board->port1.indicator.pwm, PWM_LED_RESOLUTION);
//  pwmEnable(board->port1.indicator.pwm);

//  errorIndicatorInit(&board->error, ERROR_LED, MAX_BLINKS);
  board->error = init(LedIndicator, &errorLedConfig);
  assert(board->error);

  board->usb = init(UsbDevice, &usbConfig);
  assert(board->usb);

  const struct CompositeDeviceConfig compositeConfig = {
      .device = board->usb
  };
  board->composite = init(CompositeDevice, &compositeConfig);
  assert(board->composite);

  const struct CdcAcmConfig serialConfigs[] = {
      {
          .device = board->composite,
          .rxBuffers = 4,
          .txBuffers = 4,

          .endpoints = {
              .interrupt = 0x81,
              .rx = 0x02,
              .tx = 0x82
          }
      },
      {
          .device = board->composite,
          .rxBuffers = 4,
          .txBuffers = 4,

          .endpoints = {
              .interrupt = 0x83,
              .rx = 0x04,
              .tx = 0x84
          }
      }
  };
  board->port0.serial = init(CdcAcm, &serialConfigs[0]);
  assert(board->port0.serial);
  board->port1.serial = init(CdcAcm, &serialConfigs[1]);
  assert(board->port1.serial);

  const struct CanProxyConfig proxyConfigs[] = {
      {
          .can = board->port0.can,
          .serial = board->port0.serial,
          .timer = board->chronoTimer
      },
      {
          .can = board->port1.can,
          .serial = board->port1.serial,
          .timer = board->chronoTimer
      }
  };
  board->port0.proxy = init(CanProxy, &proxyConfigs[0]);
  assert(board->port0.proxy);
  board->port1.proxy = init(CanProxy, &proxyConfigs[1]);
  assert(board->port1.proxy);
}
/*----------------------------------------------------------------------------*/
void boardStart(struct Board *board)
{
  usbDevSetConnected(board->composite, true);
  timerEnable(board->eventTimer);
}
