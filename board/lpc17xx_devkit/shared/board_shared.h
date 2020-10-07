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
void boardSetupClock(void);
struct Interface *boardSetupEeprom(struct Interface *);
struct Interface *boardSetupI2C(void);
struct Entity *boardSetupUsb(const struct SerialNumber *);
/*----------------------------------------------------------------------------*/
#endif /* BOARD_LPC17XX_DEVKIT_SHARED_BOARD_SHARED_H_ */
