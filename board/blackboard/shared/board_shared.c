/*
 * board/blackboard/shared/board_shared.c
 * Copyright (C) 2026 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board_shared.h"
#include "version.h"
#include <dpm/memory/m24.h>
#include <halm/core/cortex/systick.h>
#include <halm/generic/work_queue.h>
#include <halm/platform/stm32/can.h>
#include <halm/platform/stm32/clocking.h>
#include <halm/platform/stm32/i2c.h>
#include <halm/platform/stm32/iwdg.h>
#include <halm/platform/stm32/gptimer.h>
#include <halm/platform/stm32/usb_device.h>
#include <halm/usb/cdc_acm.h>
#include <halm/usb/usb.h>
#include <halm/usb/usb_string.h>
#include <string.h>
/*----------------------------------------------------------------------------*/
/* STM32 has 16 priority levels */

#define PRI_CHRONO  5
#define PRI_CAN     4
/* PRI_DMA 3 */
#define PRI_USB     2
#define PRI_I2C     1
#define PRI_TIMER   1
/* PRI_WQ_LP 0 */
/*----------------------------------------------------------------------------*/
DECLARE_WQ_IRQ(WQ_LP, FLASH_ISR)
/*----------------------------------------------------------------------------*/
static void customStringHeader(const void *, enum UsbLangId,
    struct UsbDescriptor *, void *);
static void customStringWrapper(const void *, enum UsbLangId,
    struct UsbDescriptor *, void *);
/*----------------------------------------------------------------------------*/
static void customStringHeader(const void *, enum UsbLangId,
    struct UsbDescriptor *header, void *payload)
{
  usbStringHeader(header, payload, LANGID_ENGLISH_US);
}
/*----------------------------------------------------------------------------*/
static void customStringWrapper(const void *argument, enum UsbLangId,
    struct UsbDescriptor *header, void *payload)
{
  usbStringWrap(header, payload, argument);
}
/*----------------------------------------------------------------------------*/
bool boardSetupClock(void)
{
  static const struct BusClockConfig apbClockConfigFast = {
      .divisor = 2
  };
  static const struct BusClockConfig apbClockConfigSlow = {
      .divisor = 4
  };
  static const struct ExternalOscConfig extOscConfig = {
      .frequency = 8000000
  };
  static const struct MainClockConfig mainClockConfig = {
      .divisor = 1,
      .range = VR_2V7_3V6
  };
  static const struct PllConfig mainPllConfig = {
      .divisor = 2,
      .multiplier = 42,
      .source = CLOCK_EXTERNAL
  };
  static const struct SystemClockConfig systemClockConfigPll = {
      .source = CLOCK_PLL
  };

  if (clockEnable(ExternalOsc, &extOscConfig) != E_OK)
    return false;
  while (!clockReady(ExternalOsc));

  if (clockEnable(MainPll, &mainPllConfig) != E_OK)
    return false;
  while (!clockReady(MainPll));

  clockEnable(Apb1Clock, &apbClockConfigSlow);
  clockEnable(Apb2Clock, &apbClockConfigFast);
  clockEnable(SystemClock, &systemClockConfigPll);

  clockEnable(MainClock, &mainClockConfig);
  return true;
}
/*----------------------------------------------------------------------------*/
void boardSetupDefaultWQ(void)
{
  static const struct WorkQueueConfig wqConfig = {
      .size = 3
  };

  WQ_DEFAULT = init(WorkQueue, &wqConfig);
}
/*----------------------------------------------------------------------------*/
void boardSetupLowPriorityWQ(void)
{
  static const struct WorkQueueIrqConfig wqConfig = {
      .size = 1,
      .irq = FLASH_IRQ,
      .priority = 0
  };

  WQ_LP = init(WorkQueueIrq, &wqConfig);
}
/*----------------------------------------------------------------------------*/
struct Interface *boardMakeCan(void)
{
  static const struct CanConfig canConfig = {
      .rate = 100000,
      .rxBuffers = 32,
      /* TX buffer count should be at least SERIALIZED_QUEUE_SIZE */
      .txBuffers = 32,
      .rx = PIN(PORT_B, 8),
      .tx = PIN(PORT_B, 9),
      .channel = CAN1
  };

  return init(Can, &canConfig);
}
/*----------------------------------------------------------------------------*/
struct Timer *boardMakeChronoTimer(void)
{
  static const struct GpTimerConfig chronoTimerConfig = {
      .frequency = 1000000,
      .priority = PRI_CHRONO,
      .channel = TIM2
  };

  return init(GpTimer, &chronoTimerConfig);
}
/*----------------------------------------------------------------------------*/
struct Timer *boardMakeEventTimer(void)
{
  return init(SysTick, &(struct SysTickConfig){PRI_TIMER});
}
/*----------------------------------------------------------------------------*/
struct Timer *boardMakeMemoryTimer(void)
{
  static const struct GpTimerConfig eepromTimerConfig = {
      .frequency = 1000000,
      .priority = PRI_I2C,
      .channel = TIM5
  };

  return init(GpTimer, &eepromTimerConfig);
}
/*----------------------------------------------------------------------------*/
struct Interface *boardMakeI2C(void)
{
  static const struct I2CConfig i2cConfig = {
      .rate = 400000,
      .scl = PIN(PORT_B, 6),
      .sda = PIN(PORT_B, 7),
      .channel = I2C1,
      .rxDma = DMA1_STREAM0,
      .txDma = DMA1_STREAM6
  };

  return init(I2C, &i2cConfig);
}
/*----------------------------------------------------------------------------*/
struct Interface *boardMakeSerial(struct Usb *usb)
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
struct Usb *boardMakeUsb()
{
  static const struct UsbDeviceConfig usbConfig = {
      .dm = PIN(PORT_A, 11),
      .dp = PIN(PORT_A, 12),
      .vid = 0x15A2,
      .pid = 0x0044,
      .priority = PRI_USB,
      .channel = 0
  };

  return init(UsbDevice, &usbConfig);
}
/*----------------------------------------------------------------------------*/
void boardMakeUsbStrings(struct Usb *usb, const char *number)
{
  const IrqState state = irqSave();

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
  if (number != NULL && strlen(number) > 0)
  {
    usbDevStringAppend(usb, usbStringBuild(customStringWrapper,
        number, USB_STRING_SERIAL, 0));
  }

  irqRestore(state);
}
/*----------------------------------------------------------------------------*/
struct Watchdog *boardMakeWatchdog(void)
{
  static const struct IwdgConfig iwdgConfig = {
      .period = 1000
  };

  clockEnable(InternalLowSpeedOsc, NULL);
  while (!clockReady(InternalLowSpeedOsc));

  return init(Iwdg, &iwdgConfig);
}
/*----------------------------------------------------------------------------*/
bool boardSetupMemoryPackage(struct MemoryPackage *package)
{
  struct Interface * const i2c = boardMakeI2C();
  struct Timer * const timer = boardMakeMemoryTimer();

  if (i2c == NULL || timer == NULL)
    return false;

  const struct M24Config memoryConfig = {
      .bus = i2c,
      .timer = timer,
      .address = 0x50,
      .chipSize = 8192,
      .pageSize = 32,
      .rate = 0,
      .blocks = 1
  };
  struct Interface * const memory = init(M24, &memoryConfig);

  if (memory != NULL)
  {
    m24SetUpdateWorkQueue(memory, WQ_LP);

    package->i2c = i2c;
    package->timer = timer;
    package->memory = memory;
    return true;
  }
  else
    return false;
}
