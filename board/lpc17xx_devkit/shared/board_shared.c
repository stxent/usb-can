/*
 * board/lpc17xx_devkit/shared/board_shared.c
 * Copyright (C) 2019 xent
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
    .rate = 400000,
    .scl = PIN(0, 11),
    .sda = PIN(0, 10),
    .priority = PRI_I2C,
    .channel = 2
};

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

static const struct ExternalOscConfig extOscConfig = {
    .frequency = 12000000
};

static const struct PllConfig sysPllConfig = {
    .source = CLOCK_EXTERNAL,
    .divisor = 3,
    .multiplier = 25
};

static const struct PllConfig usbPllConfig = {
    .source = CLOCK_EXTERNAL,
    .divisor = 4,
    .multiplier = 16
};

static const struct GenericClockConfig mainClockConfig = {
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

  res = clockEnable(ExternalOsc, &extOscConfig);
  assert(res == E_OK);
  while (!clockReady(ExternalOsc));

  res = clockEnable(SystemPll, &sysPllConfig);
  assert(res == E_OK);
  while (!clockReady(SystemPll));

  res = clockEnable(UsbPll, &usbPllConfig);
  assert(res == E_OK);
  while (!clockReady(UsbPll));

  res = clockEnable(UsbClock, &usbClockConfig);
  assert(res == E_OK);
  while (!clockReady(UsbClock));

  clockEnable(MainClock, &mainClockConfig);

  /* Suppress warning */
  (void)res;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupEeprom(struct Interface *i2c)
{
  const struct Eeprom24xxConfig eepromConfig = {
      .i2c = i2c,
      .chipSize = 8192,
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
