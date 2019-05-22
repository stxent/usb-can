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
#include <halm/platform/nxp/lpc17xx/clocking.h>
#include <halm/platform/nxp/usb_device.h>
#include <halm/usb/dfu.h>
#include "flash_loader.h"
/*----------------------------------------------------------------------------*/
#define BOOT_PIN        PIN(0, 7)

//#define FIRMWARE_OFFSET 0x4000
#define FIRMWARE_OFFSET 0x10000
#define TRANSFER_SIZE   128

#define WORK_QUEUE_SIZE 2
/*----------------------------------------------------------------------------*/
static void setupClock(void);
static void startFirmware(void);
/*----------------------------------------------------------------------------*/
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
static struct GpTimerConfig timerConfig = {
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

static const struct ExternalOscConfig extOscConfig = {
    .frequency = 12000000
};

static const struct PllConfig sysPllConfig = {
    .source = CLOCK_EXTERNAL,
    .divisor = 4,
    .multiplier = 32
};

static const struct GenericClockConfig mainClockConfig = {
    .source = CLOCK_PLL
};
/*----------------------------------------------------------------------------*/
static void setupClock(void)
{
  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(SystemPll, &sysPllConfig);
  while (!clockReady(SystemPll));

  clockEnable(MainClock, &mainClockConfig);

  clockEnable(UsbClock, &mainClockConfig);
  while (!clockReady(UsbClock));
}
/*----------------------------------------------------------------------------*/
static void startFirmware(void)
{
  const uint32_t * const table = (const uint32_t *)FIRMWARE_OFFSET;

  if (((table[0] >= 0x10000000 && table[0] <= 0x10008000)
      || (table[0] >= 0x2007C000 && table[0] <= 0x20084000))
      && table[1] <= 0x00080000)
  {
    void (*resetVector)(void) = (void (*)(void))table[1];

    nvicSetVectorTableOffset(FIRMWARE_OFFSET);
    __setMainStackPointer(table[0]);
    resetVector();
  }
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  const struct Pin bootPin = pinInit(BOOT_PIN);
  pinInput(bootPin);
  pinSetPull(bootPin, PIN_PULLUP);

  if (pinRead(bootPin))
    startFirmware();

  setupClock();

  const struct Pin ledCan = pinInit(PIN(2, 0));
  pinOutput(ledCan, false);
  const struct Pin ledFlt = pinInit(PIN(2, 1));
  pinOutput(ledFlt, true);
  const struct Pin ledCanFT = pinInit(PIN(2, 2));
  pinOutput(ledCanFT, false);
  const struct Pin ledStat = pinInit(PIN(2, 3));
  pinOutput(ledStat, false);

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
      .offset = FIRMWARE_OFFSET,
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
