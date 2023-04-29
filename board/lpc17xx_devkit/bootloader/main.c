/*
 * board/lpc17xx_devkit/bootloader/main.c
 * Copyright (C) 2018 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board_shared.h"
#include <dpm/usb/dfu_bridge.h>
#include <halm/core/cortex/nvic.h>
#include <halm/generic/work_queue.h>
#include <halm/pin.h>
#include <halm/platform/lpc/backup_domain.h>
#include <halm/platform/lpc/flash.h>
#include <halm/platform/lpc/gptimer.h>
#include <halm/platform/lpc/lpc17xx/system_defs.h>
#include <halm/usb/dfu.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
#define MAGIC_WORD    0x3A84508FUL
#define TRANSFER_SIZE 128
/*----------------------------------------------------------------------------*/
static void clearResetRequest(void);
static bool isBootloaderRequested(void);
static void onResetRequest(void);
static void startFirmware(void);
/*----------------------------------------------------------------------------*/
extern const uint32_t _firmware[2];

static const struct FlashGeometry geometry[] = {
    {
        .count = 16,
        .size = 4096,
        .time = 100
    }, {
        .count = 14,
        .size = 32768,
        .time = 100
    }, {
        /* End of the list */
        0, 0, 0
    }
};

static const struct WorkQueueConfig workQueueConfig = {
    .size = 2
};
/*----------------------------------------------------------------------------*/
static const PinNumber ledPinNumber = PIN(1, 10);

static const struct GpTimerConfig timerConfig = {
    .frequency = 1000,
    .channel = 0
};
/*----------------------------------------------------------------------------*/
static void clearResetRequest(void)
{
  LPC_SC->RSID = RSID_POR;
  *(uint32_t *)backupDomainAddress() = 0;
}
/*----------------------------------------------------------------------------*/
static bool isBootloaderRequested(void)
{
  const bool hwReset = (LPC_SC->RSID & RSID_POR) == 0;
  const bool fwRequest = *(const uint32_t *)backupDomainAddress() == MAGIC_WORD;

  return hwReset && !fwRequest;
}
/*----------------------------------------------------------------------------*/
static void onResetRequest(void)
{
  *(uint32_t *)backupDomainAddress() = MAGIC_WORD;
  nvicResetCore();
}
/*----------------------------------------------------------------------------*/
static void startFirmware(void)
{
  const uint32_t * const table = _firmware;

  if (((table[0] >= 0x10000000 && table[0] <= 0x10008000)
          || (table[0] >= 0x2007C000 && table[0] <= 0x20084000))
      && table[1] <= 0x00080000)
  {
    void (*resetVector)(void) = (void (*)(void))table[1];

    nvicSetVectorTableOffset((uint32_t)table);
    __setMainStackPointer(table[0]);
    resetVector();
  }
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  if (!isBootloaderRequested())
  {
    clearResetRequest();
    startFirmware();
  }

  boardSetupClock();

  const struct Pin led = pinInit(ledPinNumber);
  pinOutput(led, true);

  struct Interface * const flash = init(Flash, NULL);
  assert(flash != NULL);

  struct Timer * const timer = init(GpTimer, &timerConfig);
  assert(timer != NULL);

  /* USB */
  struct Entity * const usb = boardSetupUsb(NULL);
  assert(usb != NULL);

  const struct DfuConfig dfuConfig = {
      .device = usb,
      .timer = timer,
      .transferSize = TRANSFER_SIZE
  };
  struct Dfu * const dfu = init(Dfu, &dfuConfig);
  assert(dfu != NULL);

  const struct DfuBridgeConfig bridgeConfig = {
      .geometry = geometry,
      .flash = flash,
      .device = dfu,
      .offset = (size_t)&_firmware,
      .reset = onResetRequest
  };
  struct DfuBridge * const bridge = init(DfuBridge, &bridgeConfig);
  assert(bridge != NULL);
  (void)bridge;

  /* Initialize Work Queue */
  WQ_DEFAULT = init(WorkQueue, &workQueueConfig);
  assert(WQ_DEFAULT != NULL);

  /* Start USB enumeration and event loop */
  usbDevSetConnected(usb, true);
  wqStart(WQ_DEFAULT);

  return 0;
}
