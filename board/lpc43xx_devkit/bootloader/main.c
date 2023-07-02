/*
 * board/lpc43xx_devkit/bootloader/main.c
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board_shared.h"
#include "dfu_defs.h"
#include <dpm/usb/dfu_bridge.h>
#include <halm/core/cortex/nvic.h>
#include <halm/generic/flash.h>
#include <halm/generic/work_queue.h>
#include <halm/pin.h>
#include <halm/platform/lpc/backup_domain.h>
#include <halm/platform/lpc/flash.h>
#include <halm/platform/lpc/gptimer.h>
#include <halm/usb/dfu.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
#define TRANSFER_SIZE 128
/*----------------------------------------------------------------------------*/
static bool isBootloaderRequested(void);
static void onResetRequest(void);
static void setBootloaderMark(void);
static void startFirmware(void);
/*----------------------------------------------------------------------------*/
extern const uint32_t _firmware[2];

static const struct FlashConfig flashConfig = {
    .bank = FLASH_BANK_A
};

static const struct WorkQueueConfig workQueueConfig = {
    .size = 2
};
/*----------------------------------------------------------------------------*/
static bool isBootloaderRequested(void)
{
  return *(const uint32_t *)backupDomainAddress() == DFU_START_REQUEST;
}
/*----------------------------------------------------------------------------*/
static void onResetRequest(void)
{
  nvicResetCore();
}
/*----------------------------------------------------------------------------*/
static void setBootloaderMark(void)
{
  *(uint32_t *)backupDomainAddress() = DFU_START_MARK;
}
/*----------------------------------------------------------------------------*/
static void startFirmware(void)
{
  const uint32_t * const table = _firmware;

  if (((table[0] >= 0x10000000UL && table[0] <= 0x10008000UL)
          || (table[0] >= 0x10080000UL && table[0] <= 0x1008A000UL)
          || (table[0] >= 0x20000000UL && table[0] <= 0x20010000UL))
      && ((table[1] >= 0x1A000000UL && table[1] < 0x1A080000UL)
          || (table[1] >= 0x1B000000UL && table[1] < 0x1B080000UL)))
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
    setBootloaderMark();
    startFirmware();
    /* Unreachable code when main firmware is correct */
  }

  setBootloaderMark();
  boardSetupClock();

  pinOutput(pinInit(BOARD_LED_R_PIN), true);
  pinOutput(pinInit(BOARD_LED_G_PIN), true);
  pinOutput(pinInit(BOARD_LED_B_PIN), true);

  struct FlashGeometry layout[2];
  struct Interface * const flash = init(Flash, &flashConfig);
  assert(flash != NULL);
  const size_t regions = flashGetGeometry(flash, layout, ARRAY_SIZE(layout));

  struct Timer * const timer = boardMakeChronoTimer();
  assert(timer != NULL);

  /* USB */
  struct Entity * const usb = boardMakeUsb(NULL);
  assert(usb != NULL);

  const struct DfuConfig dfuConfig = {
      .device = usb,
      .timer = timer,
      .transferSize = TRANSFER_SIZE
  };
  struct Dfu * const dfu = init(Dfu, &dfuConfig);
  assert(dfu != NULL);

  const struct DfuBridgeConfig bridgeConfig = {
      .device = dfu,
      .reset = onResetRequest,
      .flash = flash,
      .offset = (size_t)&_firmware,
      .geometry = layout,
      .regions = regions,
      .writeonly = false
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
