/*
 * board/lpc17xx_devkit/shared/board_shared.c
 * Copyright (C) 2019 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board_shared.h"
#include "param_storage.h"
#include "version.h"
#include <dpm/memory/m24.h>
#include <halm/core/cortex/systick.h>
#include <halm/platform/lpc/can.h>
#include <halm/platform/lpc/clocking.h>
#include <halm/platform/lpc/i2c.h>
#include <halm/platform/lpc/gptimer.h>
#include <halm/platform/lpc/usb_device.h>
#include <halm/platform/lpc/wdt.h>
#include <halm/usb/cdc_acm.h>
#include <halm/usb/usb.h>
#include <halm/usb/usb_string.h>
#include <assert.h>
#include <string.h>
/*----------------------------------------------------------------------------*/
/* LPC17xx has 32 priority levels */

#define PRI_CHRONO  5
#define PRI_CAN     4
/* PRI_GPDMA 3 */
#define PRI_USB     2
#define PRI_I2C     1
#define PRI_TIMER   1
/* PRI_WQ_LP 0 */
/*----------------------------------------------------------------------------*/
static void customStringHeader(const void *, enum UsbLangId,
    struct UsbDescriptor *, void *);
static void customStringWrapper(const void *, enum UsbLangId,
    struct UsbDescriptor *, void *);
/*----------------------------------------------------------------------------*/
static void customStringHeader(const void *argument __attribute__((unused)),
    enum UsbLangId langid __attribute__((unused)),
    struct UsbDescriptor *header, void *payload)
{
  usbStringHeader(header, payload, LANGID_ENGLISH_US);
}
/*----------------------------------------------------------------------------*/
static void customStringWrapper(const void *argument,
    enum UsbLangId langid __attribute__((unused)),
    struct UsbDescriptor *header, void *payload)
{
  usbStringWrap(header, payload, argument);
}
/*----------------------------------------------------------------------------*/
bool boardSetupClock(void)
{
  static const struct ExternalOscConfig extOscConfig = {
      .frequency = 12000000
  };
  static const struct PllConfig sysPllConfig = {
      .divisor = 3,
      .multiplier = 25,
      .source = CLOCK_EXTERNAL
  };
  static const struct PllConfig usbPllConfig = {
      .divisor = 4,
      .multiplier = 16,
      .source = CLOCK_EXTERNAL
  };

  if (clockEnable(ExternalOsc, &extOscConfig) != E_OK)
    return false;
  while (!clockReady(ExternalOsc));

  if (clockEnable(SystemPll, &sysPllConfig) != E_OK)
    return false;
  while (!clockReady(SystemPll));

  if (clockEnable(UsbPll, &usbPllConfig) != E_OK)
    return false;
  while (!clockReady(UsbPll));

  clockEnable(UsbClock, &(struct GenericClockConfig){CLOCK_USB_PLL});
  clockEnable(MainClock, &(struct GenericClockConfig){CLOCK_PLL});

  return true;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardMakeCan(void)
{
  static const struct CanConfig canConfig = {
      .rate = 10000,
      .rxBuffers = 32,
      /* TX buffer count should be at least SERIALIZED_QUEUE_SIZE */
      .txBuffers = 32,
      .rx = PIN(0, 0),
      .tx = PIN(0, 1),
      .priority = PRI_CAN,
      .channel = 0
  };

  return init(Can, &canConfig);
}
/*----------------------------------------------------------------------------*/
struct Timer *boardMakeChronoTimer(void)
{
  static const struct GpTimerConfig chronoTimerConfig = {
      .frequency = 1000000,
      .priority = PRI_CHRONO,
      .channel = 0
  };

  return init(GpTimer, &chronoTimerConfig);
}
/*----------------------------------------------------------------------------*/
struct Interface *boardMakeEeprom(struct Interface *bus, struct Timer *timer)
{
  const struct M24Config config = {
      .bus = bus,
      .timer = timer,
      .address = 0x50,
      .chipSize = 8192,
      .pageSize = 32,
      .rate = 0, /* Use default rate */
      .blocks = 1
  };

  return init(M24, &config);
}
/*----------------------------------------------------------------------------*/
struct Timer *boardMakeEepromTimer(void)
{
  static const struct GpTimerConfig eepromTimerConfig = {
      .frequency = 1000000,
      .priority = PRI_I2C,
      .channel = 1
  };

  return init(GpTimer, &eepromTimerConfig);
}
/*----------------------------------------------------------------------------*/
struct Timer *boardMakeEventTimer(void)
{
  return init(SysTick, &(struct SysTickConfig){PRI_TIMER});
}
/*----------------------------------------------------------------------------*/
struct Interface *boardMakeI2C(void)
{
  static const struct I2CConfig i2cConfig = {
      .rate = 400000,
      .scl = PIN(0, 11),
      .sda = PIN(0, 10),
      .priority = PRI_I2C,
      .channel = 2
  };

  return init(I2C, &i2cConfig);
}
/*----------------------------------------------------------------------------*/
struct Interface *boardMakeSerial(struct Entity *usb)
{
  /* CDC */
  const struct CdcAcmConfig config = {
      .device = usb,
      .rxBuffers = 4,
      .txBuffers = 8,

      .endpoints = {
          .interrupt = 0x81,
          .rx = 0x02,
          .tx = 0x82
      }
  };

  return init(CdcAcm, &config);
}
/*----------------------------------------------------------------------------*/
struct Entity *boardMakeUsb(const struct SerialNumber *number)
{
  static const struct UsbDeviceConfig usbConfig = {
      .dm = PIN(0, 30),
      .dp = PIN(0, 29),
      .connect = PIN(2, 9),
      .vbus = PIN(1, 30),
      .vid = 0x2404,
      .pid = 0x03EB,
      .priority = PRI_USB,
      .channel = 0
  };

  /* USB Device */
  struct Entity * const usb = init(UsbDevice, &usbConfig);

  if (usb == NULL)
    return NULL;

  /* USB Strings */
  usbDevStringAppend(usb, usbStringBuild(customStringHeader, NULL,
      USB_STRING_HEADER, 0));

  if (strlen(getUsbVendorString()) > 0)
  {
    usbDevStringAppend(usb, usbStringBuild(customStringWrapper,
        getUsbVendorString(), USB_STRING_VENDOR, 0));
  }
  if (strlen(getUsbProductString()) > 0)
  {
    usbDevStringAppend(usb, usbStringBuild(customStringWrapper,
        getUsbProductString(), USB_STRING_PRODUCT, 0));
  }
  if (number != NULL && strlen(number->value) > 0)
  {
    usbDevStringAppend(usb, usbStringBuild(customStringWrapper,
        number->value, USB_STRING_SERIAL, 0));
  }

  return usb;
}
/*----------------------------------------------------------------------------*/
struct Watchdog *boardMakeWatchdog(void)
{
  static const struct WdtConfig wdtConfig = {
      .period = 1000
  };

  return init(Wdt, &wdtConfig);
}
