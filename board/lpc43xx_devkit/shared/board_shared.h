/*
 * board/lpc43xx_devkit/shared/board_shared.h
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef BOARD_LPC43XX_DEVKIT_SHARED_BOARD_SHARED_H_
#define BOARD_LPC43XX_DEVKIT_SHARED_BOARD_SHARED_H_
/*----------------------------------------------------------------------------*/
#include <halm/generic/work_queue_irq.h>
#include <halm/pin.h>
/*----------------------------------------------------------------------------*/
#define BOARD_LED_R_PIN PIN(PORT_5, 7)
#define BOARD_LED_G_PIN PIN(PORT_5, 5)
#define BOARD_LED_B_PIN PIN(PORT_4, 0)

#define BOARD_LED_BUSY  BOARD_LED_G_PIN
#define BOARD_LED_ERROR BOARD_LED_R_PIN
/*----------------------------------------------------------------------------*/
struct Interface;
struct Timer;
struct Usb;
struct Watchdog;

struct MemoryPackage
{
  struct Interface *i2c;
  struct Interface *memory;
  struct Timer *timer;
};

DEFINE_WQ_IRQ(WQ_LP)
/*----------------------------------------------------------------------------*/
bool boardSetupClock(void);
void boardSetupDefaultWQ(void);
void boardSetupLowPriorityWQ(void);

struct Interface *boardMakeCan(void);
struct Timer *boardMakeChronoTimer(void);
struct Timer *boardMakeEventTimer(void);
struct Timer *boardMakeMemoryTimer(void);
struct Interface *boardMakeI2C(void);
struct Interface *boardMakeSerial(struct Usb *);
struct Usb *boardMakeUsb(void);
void boardMakeUsbStrings(struct Usb *, const char *);
struct Watchdog *boardMakeWatchdog(void);

bool boardSetupMemoryPackage(struct MemoryPackage *);
/*----------------------------------------------------------------------------*/
#endif /* BOARD_LPC43XX_DEVKIT_SHARED_BOARD_SHARED_H_ */
