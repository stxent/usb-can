/*
 * board/blackboard/shared/board_shared.h
 * Copyright (C) 2026 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef BOARD_BLACKBOARD_SHARED_BOARD_SHARED_H_
#define BOARD_BLACKBOARD_SHARED_BOARD_SHARED_H_
/*----------------------------------------------------------------------------*/
#include <halm/generic/work_queue_irq.h>
#include <halm/pin.h>
/*----------------------------------------------------------------------------*/
#define BOARD_LED_BUSY  PIN(PORT_F, 9)
#define BOARD_LED_ERROR PIN(PORT_F, 10)
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
#endif /* BOARD_BLACKBOARD_SHARED_BOARD_SHARED_H_ */
