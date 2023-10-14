/*
 * board/lpc43xx_devkit/shared/board_shared.c
 * Copyright (C) 2023 xent
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
#include <string.h>
/*----------------------------------------------------------------------------*/
/* LPC43xx has 8 priority levels */

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
  static const struct PllConfig audioPllConfig = {
      .divisor = 4,
      .multiplier = 40,
      .source = CLOCK_EXTERNAL
  };
  static const struct GenericDividerConfig divCConfig = {
      .divisor = 3,
      .source = CLOCK_AUDIO_PLL
  };
  static const struct ExternalOscConfig extOscConfig = {
      .frequency = 12000000
  };
  static const struct PllConfig sysPllConfig = {
      .divisor = 2,
      .multiplier = 17,
      .source = CLOCK_EXTERNAL
  };
  static const struct PllConfig usbPllConfig = {
      .divisor = 1,
      .multiplier = 40,
      .source = CLOCK_EXTERNAL
  };

  clockEnable(MainClock, &(struct GenericClockConfig){CLOCK_INTERNAL});

  if (clockEnable(ExternalOsc, &extOscConfig) != E_OK)
    return false;
  while (!clockReady(ExternalOsc));

  /* Make 120 MHz clock on AUDIO PLL */
  if (clockEnable(AudioPll, &audioPllConfig) != E_OK)
    return false;
  while (!clockReady(AudioPll));

  if (clockEnable(SystemPll, &sysPllConfig) != E_OK)
    return false;
  while (!clockReady(SystemPll));

  if (clockEnable(UsbPll, &usbPllConfig) != E_OK)
    return false;
  while (!clockReady(UsbPll));

  /* Make 40 MHz clock for CAN, clock should be less than 50 MHz */
  if (clockEnable(DividerC, &divCConfig) != E_OK)
    return false;
  while (!clockReady(DividerC));

  /* CAN0 is connected to the APB3 bus */
  clockEnable(Apb3Clock, &(struct GenericClockConfig){CLOCK_IDIVC});
  /* CAN1 is connected to the APB1 bus */
  clockEnable(Apb1Clock, &(struct GenericClockConfig){CLOCK_IDIVC});

  clockEnable(Usb0Clock, &(struct GenericClockConfig){CLOCK_USB_PLL});
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
      .rx = PIN(PORT_3, 1),
      .tx = PIN(PORT_3, 2),
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
      .chipSize = 65536,
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
      .rate = 100000,
      .scl = PIN(PORT_2, 4),
      .sda = PIN(PORT_2, 3),
      .priority = PRI_I2C,
      .channel = 1
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
      .txBuffers = 4,

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
      .dm = PIN(PORT_USB, PIN_USB0_DM),
      .dp = PIN(PORT_USB, PIN_USB0_DP),
      .connect = 0,
      .vbus = PIN(PORT_USB, PIN_USB0_VBUS),
      .vid = 0x15A2,
      .pid = 0x0044,
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
