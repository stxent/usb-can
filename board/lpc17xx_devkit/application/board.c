/*
 * board/lpc17xx_devkit/application/board.c
 * Copyright (C) 2019 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include "board_shared.h"
#include "dfu_defs.h"
#include "led_indicator.h"
#include <dpm/memory/m24.h>
#include <halm/core/cortex/nvic.h>
#include <halm/delay.h>
#include <halm/generic/work_queue.h>
#include <halm/generic/work_queue_irq.h>
#include <halm/platform/lpc/backup_domain.h>
#include <halm/usb/usb.h>
#include <halm/watchdog.h>
/*----------------------------------------------------------------------------*/
#define EVENT_RATE    50
#define MAX_BLINKS    16

#define MEMORY_OFFSET 0
/*----------------------------------------------------------------------------*/
static void panic(struct Pin, struct Watchdog *);
/*----------------------------------------------------------------------------*/
DECLARE_WQ_IRQ(WQ_LP, SPI_ISR)
/*----------------------------------------------------------------------------*/
static const struct LedIndicatorConfig errorLedConfig = {
    .pin = BOARD_LED_ERROR,
    .limit = MAX_BLINKS,
    .inversion = false
};

static const struct LedIndicatorConfig portLedConfig = {
    .pin = BOARD_LED_BUSY,
    .limit = MAX_BLINKS,
    .inversion = true
};

static const struct WorkQueueConfig workQueueConfig = {
    .size = 3
};

static const struct WorkQueueIrqConfig workQueueIrqConfig = {
    .size = 1,
    .irq = SPI_IRQ,
    .priority = 0
};
/*----------------------------------------------------------------------------*/
static void panic(struct Pin led, struct Watchdog *watchdog)
{
  while (1)
  {
    if (watchdog != NULL)
      watchdogReload(watchdog);

    pinToggle(led);
    mdelay(500);
  }
}
/*----------------------------------------------------------------------------*/
void appBoardInit(struct Board *board)
{
  struct Pin led = pinInit(BOARD_LED_ERROR);
  pinOutput(led, false);

  if (!boardSetupClock())
    panic(led, NULL);

#ifdef ENABLE_WDT
  board->watchdog = boardMakeWatchdog();
  if (board->watchdog == NULL)
    panic(led, NULL);
#else
  board->watchdog = NULL;
#endif

  /* Initialize Work Queues */

  WQ_DEFAULT = init(WorkQueue, &workQueueConfig);
  if (WQ_DEFAULT == NULL)
    panic(led, board->watchdog);
  WQ_LP = init(WorkQueueIrq, &workQueueIrqConfig);
  if (WQ_LP == NULL)
    panic(led, board->watchdog);

  /* Timers */

  board->chronoTimer = boardMakeChronoTimer();
  if (board->chronoTimer == NULL)
    panic(led, board->watchdog);

  board->eepromTimer = boardMakeEepromTimer();
  if (board->eepromTimer == NULL)
    panic(led, board->watchdog);

  board->eventTimer = boardMakeEventTimer();
  if (board->eventTimer == NULL)
    panic(led, board->watchdog);

  /* I2C and parameter storage, start Low-Priority Work Queue */

  board->i2c = boardMakeI2C();
  if (board->i2c == NULL)
    panic(led, board->watchdog);
  board->eeprom = boardMakeEeprom(board->i2c, board->eepromTimer);
  if (board->eeprom == NULL)
    panic(led, board->watchdog);

  wqStart(WQ_LP);
  m24SetUpdateWorkQueue(board->eeprom, WQ_LP);

  storageInit(&board->storage, board->eeprom, MEMORY_OFFSET);
  storageLoad(&board->storage);
  makeSerialNumber(&board->number, board->storage.values.serial);

  /* CAN */

  board->can = boardMakeCan();
  if (board->can == NULL)
    panic(led, board->watchdog);

  /* USB */

  board->usb = boardMakeUsb(&board->number);
  if (board->usb == NULL)
    panic(led, board->watchdog);
  board->serial = boardMakeSerial(board->usb);
  if (board->serial == NULL)
    panic(led, board->watchdog);

  /* Indication */

  board->error = init(LedIndicator, &errorLedConfig);
  if (board->error == NULL)
    panic(led, board->watchdog);
  board->status = init(LedIndicator, &portLedConfig);
  if (board->status == NULL)
    panic(led, board->watchdog);

  /* Create port hub and initialize all ports */

  board->hub = makeProxyHub(1);
  if (board->hub == NULL)
    panic(led, board->watchdog);

  const struct ProxyPortConfig proxyPortConfig = {
      .can = board->can,
      .serial = board->serial,
      .chrono = board->chronoTimer,
      .error = board->error,
      .status = board->status,
      .storage = &board->storage
  };
  if (!proxyPortInit(&board->hub->ports[0], &proxyPortConfig))
    panic(led, board->watchdog);
}
/*----------------------------------------------------------------------------*/
int appBoardStart(struct Board *board)
{
  usbDevSetConnected(board->usb, true);

  timerSetOverflow(board->eventTimer,
      timerGetFrequency(board->eventTimer) / EVENT_RATE);

  timerEnable(board->chronoTimer);
  timerEnable(board->eventTimer);

  wqStart(WQ_DEFAULT);
  return 0;
}
/*----------------------------------------------------------------------------*/
void resetToBootloader(void)
{
  *(uint32_t *)backupDomainAddress() = DFU_START_REQUEST;
  nvicResetCore();
}
