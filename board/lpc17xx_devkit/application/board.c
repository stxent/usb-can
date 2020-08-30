/*
 * board/lpc17xx_devkit/application/board.c
 * Copyright (C) 2019 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <halm/core/cortex/systick.h>
#include <halm/pin.h>
#include <halm/platform/nxp/can.h>
#include <halm/platform/nxp/gptimer.h>
#include <halm/usb/cdc_acm.h>
#include "board.h"
#include "board_shared.h"
#include "helpers.h"
#include "led_indicator.h"
#include "version.h"
/*----------------------------------------------------------------------------*/
#define EVENT_RATE 50
#define MAX_BLINKS 16
/*----------------------------------------------------------------------------*/
static const struct CanConfig canConfig = {
    .rate = 10000,
    .rxBuffers = 16,
    .txBuffers = 16,
    .rx = PIN(0, 0),
    .tx = PIN(0, 1),
    .priority = 3,
    .channel = 0
};

static const struct SysTickTimerConfig eventTimerConfig = {
    .priority = 0
};

static const struct GpTimerConfig chronoTimerConfig = {
    .frequency = 1000000,
    .channel = 0
};

static const struct LedIndicatorConfig errorLedConfig = {
    .pin = PIN(1, 10),
    .limit = MAX_BLINKS,
    .inversion = false
};

static const struct LedIndicatorConfig portLedConfig = {
    .pin = PIN(1, 9),
    .limit = MAX_BLINKS,
    .inversion = true
};
/*----------------------------------------------------------------------------*/
static void boardSetupSerial(struct Board *board)
{
  /* CDC */
  const struct CdcAcmConfig serialConfig = {
      .device = board->usb,
      .rxBuffers = 4,
      .txBuffers = 4,

      .endpoints = {
          .interrupt = 0x81,
          .rx = 0x02,
          .tx = 0x82
      }
  };
  board->serial = init(CdcAcm, &serialConfig);
  assert(board->serial);
}
/*----------------------------------------------------------------------------*/
void boardSetup(struct Board *board)
{
  /* I2C and parameter storage*/
  board->i2c = boardSetupI2C();
  assert(board->i2c);
  board->eeprom = boardSetupEeprom(board->i2c);
  assert(board->eeprom);

  storageInit(&board->storage, board->eeprom, 0);
  storageLoad(&board->storage);
  makeSerialNumber(&board->number, board->storage.values.serial);

  /* USB */
  board->usb = boardSetupUsb(&board->number);
  assert(board->usb);
  boardSetupSerial(board);

  /* Other peripherals */
  board->chronoTimer = init(GpTimer, &chronoTimerConfig);
  assert(board->chronoTimer);
  timerEnable(board->chronoTimer);

  board->eventTimer = init(SysTickTimer, &eventTimerConfig);
  assert(board->eventTimer);
  timerSetOverflow(board->eventTimer,
      timerGetFrequency(board->eventTimer) / EVENT_RATE);

  board->error = init(LedIndicator, &errorLedConfig);
  assert(board->error);
  board->status = init(LedIndicator, &portLedConfig);
  assert(board->status);

  board->can = init(Can, &canConfig);
  assert(board->can);

  /* Create port hub and initialize all ports */
  board->hub = makeProxyHub(1);
  assert(board->hub);

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
