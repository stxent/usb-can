/*
 * platform/lpc17xx_devkit/bootloader/main.c
 * Copyright (C) 2018 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <halm/core/cortex/nvic.h>
#include <halm/generic/work_queue.h>
#include <halm/pin.h>
#include <halm/platform/nxp/flash.h>
#include <halm/platform/nxp/gptimer.h>
#include <halm/platform/nxp/usb_device.h>
#include <halm/usb/dfu.h>
#include "board_shared.h"
#include "flash_loader.h"
/*----------------------------------------------------------------------------*/
#define TRANSFER_SIZE   128
#define WORK_QUEUE_SIZE 2
/*----------------------------------------------------------------------------*/
static void startFirmware(void);
/*----------------------------------------------------------------------------*/
extern const uint32_t _firmware;

static const struct FlashGeometry geometry[] = {
    {
        .count = 16,
        .size = 4096,
        .time = 100
    },
    {
        .count = 14,
        .size = 32768,
        .time = 100
    },
    {
        /* End of the list */
        0, 0, 0
    }
};
/*----------------------------------------------------------------------------*/
static const PinNumber bootPinNumber = PIN(2, 10);
static const PinNumber ledPinNumber = PIN(1, 10);

static const struct GpTimerConfig timerConfig = {
    .frequency = 1000,
    .channel = 0
};

static const struct UsbDeviceConfig usbConfig = {
    .dm = PIN(0, 30),
    .dp = PIN(0, 29),
    .connect = PIN(2, 9),
    .vbus = PIN(1, 30),
    .vid = 0x15A2,
    .pid = 0x0044,
    .channel = 0
};
/*----------------------------------------------------------------------------*/
static void startFirmware(void)
{
  const uint32_t * const table = &_firmware;

  if (((table[0] >= 0x10000000 && table[0] <= 0x10008000)
          || (table[0] >= 0x2007C000 && table[0] <= 0x20084000))
      && table[1] <= 0x00080000)
  {
    void (*resetVector)(void) = (void (*)(void))table[1];

    nvicSetVectorTableOffset((uint32_t)&_firmware);
    __setMainStackPointer(table[0]);
    resetVector();
  }
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  const struct Pin bootPin = pinInit(bootPinNumber);
  pinInput(bootPin);

  if (pinRead(bootPin))
    startFirmware();

  boardSetupClock();

  const struct Pin led = pinInit(ledPinNumber);
  pinOutput(led, true);

  struct Interface * const flash = init(Flash, 0);
  assert(flash);

  struct Timer * const timer = init(GpTimer, &timerConfig);
  assert(timer);

  struct Entity * const usb = init(UsbDevice, &usbConfig);
  assert(usb);

  const struct DfuConfig dfuConfig = {
      .device = usb,
      .timer = timer,
      .transferSize = TRANSFER_SIZE
  };
  struct Dfu * const dfu = init(Dfu, &dfuConfig);
  assert(dfu);

  const struct FlashLoaderConfig loaderConfig = {
      .geometry = geometry,
      .flash = flash,
      .device = dfu,
      .offset = (size_t)&_firmware,
      .reset = nvicResetCore
  };
  struct FlashLoader * const loader = init(FlashLoader, &loaderConfig);
  assert(loader);
  (void)loader;

  usbDevSetConnected(usb, true);

  /* Initialize and start Work Queue */
  workQueueInit(WORK_QUEUE_SIZE);
  workQueueStart(0);

  return 0;
}
