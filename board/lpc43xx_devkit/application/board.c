/*
 * board/lpc43xx_devkit/application/board.c
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include "board_shared.h"
#include "helpers.h"
#include "led_indicator.h"
#include "version.h"
#include <halm/core/cortex/nvic.h>
#include <halm/core/cortex/systick.h>
#include <halm/pin.h>
#include <halm/platform/lpc/can.h>
#include <halm/platform/lpc/gptimer.h>
#include <halm/platform/lpc/wdt.h>
#include <halm/usb/cdc_acm.h>
#include <halm/usb/usb.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
#define EVENT_RATE 50
#define MAX_BLINKS 16
/*----------------------------------------------------------------------------*/
static const struct CanConfig canConfig = {
    .rate = 10000,
    .rxBuffers = 4,
    .txBuffers = 4,
    .rx = PIN(PORT_3, 1),
    .tx = PIN(PORT_3, 2),
    .priority = PRI_CAN,
    .channel = 0
};

static const struct SysTickConfig eventTimerConfig = {
    .priority = PRI_TIMER
};

static const struct GpTimerConfig chronoTimerConfig = {
    .frequency = 1000000,
    .priority = PRI_CHRONO,
    .channel = 0
};

static const struct LedIndicatorConfig errorLedConfig = {
    .pin = PIN(PORT_5, 7),
    .limit = MAX_BLINKS,
    .inversion = false
};

static const struct LedIndicatorConfig portLedConfig = {
    .pin = PIN(PORT_5, 5),
    .limit = MAX_BLINKS,
    .inversion = true
};

static const struct WdtConfig wdtConfig = {
    .period = 1000
};
/*----------------------------------------------------------------------------*/
static void boardSetupSerial(struct Board *board)
{
  /* CDC */
  const struct CdcAcmConfig serialConfig = {
      .device = board->usb,
      .rxBuffers = 4,
      .txBuffers = 8,

      .endpoints = {
          .interrupt = 0x81,
          .rx = 0x02,
          .tx = 0x82
      }
  };
  board->serial = init(CdcAcm, &serialConfig);
  assert(board->serial != NULL);
}
/*----------------------------------------------------------------------------*/
void boardSetup(struct Board *board)
{
#ifdef ENABLE_WDT
  board->wdt = init(Wdt, &wdtConfig);
  assert(board->wdt != NULL);
#else
  (void)wdtConfig;
  board->wdt = NULL;
#endif

  /* I2C and parameter storage*/
  board->i2c = boardSetupI2C();
  assert(board->i2c != NULL);
  board->eeprom = boardSetupEeprom(board->i2c);
  assert(board->eeprom != NULL);

  storageInit(&board->storage, board->eeprom, 0);
  storageLoad(&board->storage);
  makeSerialNumber(&board->number, board->storage.values.serial);

  /* USB */
  board->usb = boardSetupUsb(&board->number);
  assert(board->usb != NULL);
  boardSetupSerial(board);

  /* Other peripherals */
  board->chronoTimer = init(GpTimer, &chronoTimerConfig);
  assert(board->chronoTimer != NULL);
  timerEnable(board->chronoTimer);

  board->eventTimer = init(SysTick, &eventTimerConfig);
  assert(board->eventTimer != NULL);
  timerSetOverflow(board->eventTimer,
      timerGetFrequency(board->eventTimer) / EVENT_RATE);

  board->error = init(LedIndicator, &errorLedConfig);
  assert(board->error != NULL);
  board->status = init(LedIndicator, &portLedConfig);
  assert(board->status != NULL);

  board->can = init(Can, &canConfig);
  assert(board->can != NULL);

  /* Create port hub and initialize all ports */
  board->hub = makeProxyHub(1);
  assert(board->hub != NULL);

  const struct ProxyPortConfig proxyPortConfig = {
      .can = board->can,
      .serial = board->serial,
      .chrono = board->chronoTimer,
      .error = board->error,
      .status = board->status,
      .storage = &board->storage
  };
  proxyPortInit(&board->hub->ports[0], &proxyPortConfig);
}
/*----------------------------------------------------------------------------*/
void boardStart(struct Board *board)
{
  usbDevSetConnected(board->usb, true);
  timerEnable(board->eventTimer);
}
/*----------------------------------------------------------------------------*/
void resetToBootloader(void)
{
  nvicResetCore();
}
