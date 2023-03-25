/*
 * board/lpc17xx_devkit/shared/board_shared.h
 * Copyright (C) 2019 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef BOARD_LPC17XX_DEVKIT_SHARED_BOARD_SHARED_H_
#define BOARD_LPC17XX_DEVKIT_SHARED_BOARD_SHARED_H_
/*----------------------------------------------------------------------------*/
#include "param_storage.h"
#include <xcore/interface.h>
/*----------------------------------------------------------------------------*/
/* LPC17xx has 32 priority levels */

#define PRI_CHRONO  4
#define PRI_CAN     3
/* PRI_GPDMA 2 */
#define PRI_USB     1
#define PRI_I2C     0
#define PRI_TIMER   0
/*----------------------------------------------------------------------------*/
void boardSetupClock(void);
struct Interface *boardSetupEeprom(struct Interface *);
struct Interface *boardSetupI2C(void);
struct Entity *boardSetupUsb(const struct SerialNumber *);
/*----------------------------------------------------------------------------*/
#endif /* BOARD_LPC17XX_DEVKIT_SHARED_BOARD_SHARED_H_ */
