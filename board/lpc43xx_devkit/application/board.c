/*
 * board/lpc43xx_devkit/application/board.c
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include "board_shared.h"
#include "dfu_defs.h"
#include "led_indicator.h"
#include <dpm/memory/m24.h>
#include <halm/core/cortex/nvic.h>
#include <halm/generic/work_queue.h>
#include <halm/generic/work_queue_irq.h>
#include <halm/platform/lpc/backup_domain.h>
#include <halm/usb/usb.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
#define EVENT_RATE    50
#define MAX_BLINKS    16

#define MEMORY_OFFSET 0

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
void appBoardInit(struct Board *board)
{
  boardSetupClock();

#ifdef ENABLE_WDT
  board->watchdog = boardMakeWatchdog();
  assert(board->watchdog != NULL);
#else
  board->watchdog = NULL;
#endif

  /* Initialize Work Queues, start Low-Priority Work Queue */

  WQ_DEFAULT = init(WorkQueue, &workQueueConfig);
  assert(WQ_DEFAULT != NULL);
  WQ_LP = init(WorkQueueIrq, &workQueueIrqConfig);
  assert(WQ_LP != NULL);
  wqStart(WQ_LP);

  /* Timers */

  board->chronoTimer = boardMakeChronoTimer();
  assert(board->chronoTimer != NULL);

  board->eepromTimer = boardMakeEepromTimer();
  assert(board->eepromTimer != NULL);

  board->eventTimer = boardMakeEventTimer();
  assert(board->eventTimer != NULL);
  timerSetOverflow(board->eventTimer,
      timerGetFrequency(board->eventTimer) / EVENT_RATE);

  /* I2C and parameter storage */

  board->i2c = boardMakeI2C();
  assert(board->i2c != NULL);
  board->eeprom = boardMakeEeprom(board->i2c, board->eepromTimer);
  assert(board->eeprom != NULL);
  m24SetUpdateWorkQueue(board->eeprom, WQ_LP);

  storageInit(&board->storage, board->eeprom, MEMORY_OFFSET);
  storageLoad(&board->storage);
  makeSerialNumber(&board->number, board->storage.values.serial);

  /* CAN */

  board->can = boardMakeCan();
  assert(board->can != NULL);

  /* USB */

  board->usb = boardMakeUsb(&board->number);
  assert(board->usb != NULL);
  board->serial = boardMakeSerial(board->usb);
  assert(board->serial != NULL);

  /* Indication */

  board->error = init(LedIndicator, &errorLedConfig);
  assert(board->error != NULL);
  board->status = init(LedIndicator, &portLedConfig);
  assert(board->status != NULL);

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
int appBoardStart(struct Board *board)
{
  usbDevSetConnected(board->usb, true);

  timerEnable(board->chronoTimer);
  timerEnable(board->eventTimer);

  /* Start Work Queue */
  wqStart(WQ_DEFAULT);

  return 0;
}
/*----------------------------------------------------------------------------*/
void resetToBootloader(void)
{
  *(uint32_t *)backupDomainAddress() = DFU_START_REQUEST;
  nvicResetCore();
}
