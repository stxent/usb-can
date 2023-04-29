/*
 * board/lpc43xx_devkit/shared/board_shared.c
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board_shared.h"
#include "version.h"
#include <dpm/memory/eeprom_24xx.h>
#include <halm/platform/lpc/clocking.h>
#include <halm/platform/lpc/i2c.h>
#include <halm/platform/lpc/usb_device.h>
#include <halm/usb/usb.h>
#include <halm/usb/usb_string.h>
#include <assert.h>
#include <string.h>
/*----------------------------------------------------------------------------*/
static void customStringHeader(const void *, enum UsbLangId,
    struct UsbDescriptor *, void *);
static void customStringWrapper(const void *, enum UsbLangId,
    struct UsbDescriptor *, void *);
/*----------------------------------------------------------------------------*/
static const struct I2CConfig i2cConfig = {
    .rate = 100000,
    .scl = PIN(PORT_2, 4),
    .sda = PIN(PORT_2, 3),
    .priority = PRI_I2C,
    .channel = 1
};

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

static const struct PllConfig audioPllConfig = {
    .source = CLOCK_EXTERNAL,
    .divisor = 4,
    .multiplier = 40
};

static const struct GenericDividerConfig divBConfig = {
    .source = CLOCK_AUDIO_PLL,
    .divisor = 3
};

static const struct GenericClockConfig divBClockSource = {
    .source = CLOCK_IDIVB
};

static const struct ExternalOscConfig extOscConfig = {
    .frequency = 12000000
};

static const struct PllConfig sysPllConfig = {
    .source = CLOCK_EXTERNAL,
    .divisor = 2,
    .multiplier = 17
};

static const struct PllConfig usbPllConfig = {
    .source = CLOCK_EXTERNAL,
    .divisor = 1,
    .multiplier = 40
};

static const struct GenericClockConfig mainClockConfigInt = {
    .source = CLOCK_INTERNAL
};

static const struct GenericClockConfig mainClockConfigPll = {
    .source = CLOCK_PLL
};

static const struct GenericClockConfig usbClockConfig = {
    .source = CLOCK_USB_PLL
};
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
void boardSetupClock(void)
{
  enum Result res;

  clockEnable(MainClock, &mainClockConfigInt);

  res = clockEnable(ExternalOsc, &extOscConfig);
  assert(res == E_OK);
  while (!clockReady(ExternalOsc));

  /* Make 120 MHz clock on AUDIO PLL */
  res = clockEnable(AudioPll, &audioPllConfig);
  assert(res == E_OK);
  while (!clockReady(AudioPll));

  res = clockEnable(SystemPll, &sysPllConfig);
  assert(res == E_OK);
  while (!clockReady(SystemPll));

  res = clockEnable(UsbPll, &usbPllConfig);
  assert(res == E_OK);
  while (!clockReady(UsbPll));

  /* Make 40 MHz clock for CAN, clock should be less than 50 MHz */
  res = clockEnable(DividerB, &divBConfig);
  assert(res == E_OK);
  while (!clockReady(DividerB));

  /* CAN0 is connected to the APB3 bus */
  res = clockEnable(Apb3Clock, &divBClockSource);
  assert(res == E_OK);
  while (!clockReady(Apb3Clock));

  /* CAN1 is connected to the APB1 bus */
  res = clockEnable(Apb1Clock, &divBClockSource);
  assert(res == E_OK);
  while (!clockReady(Apb1Clock));

  res = clockEnable(Usb0Clock, &usbClockConfig);
  assert(res == E_OK);
  while (!clockReady(Usb0Clock));

  clockEnable(MainClock, &mainClockConfigPll);

  /* Suppress warning */
  (void)res;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupEeprom(struct Interface *i2c)
{
  const struct Eeprom24xxConfig eepromConfig = {
      .i2c = i2c,
      .chipSize = 65536,
      .rate = 0, /* Use default rate */
      .address = 0x50,
      .pageSize = 32,
      .blocks = 1
  };

  return init(Eeprom24xx, &eepromConfig);
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupI2C(void)
{
  return init(I2C, &i2cConfig);
}
/*----------------------------------------------------------------------------*/
struct Entity *boardSetupUsb(const struct SerialNumber *number)
{
  /* USB Device */
  struct Entity * const usb = init(UsbDevice, &usbConfig);

  if (usb == NULL)
    return NULL;

  /* USB Strings */
  usbDevStringAppend(usb, usbStringBuild(customStringHeader, 0,
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
  if (number && strlen(number->value) > 0)
  {
    usbDevStringAppend(usb, usbStringBuild(customStringWrapper,
        number->value, USB_STRING_SERIAL, 0));
  }

  return usb;
}
