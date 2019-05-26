/*
 * board/lpc17xx_devkit/board.h
 * Copyright (C) 2019 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef BOARD_LPC17XX_DEVKIT_APPLICATION_BOARD_H_
#define BOARD_LPC17XX_DEVKIT_APPLICATION_BOARD_H_
/*----------------------------------------------------------------------------*/
#include <halm/timer.h>
#include <xcore/interface.h>
#include "proxy_port.h"
/*----------------------------------------------------------------------------*/
struct Board
{
  struct Entity *usb;
  struct Interface *serial;
  struct Timer *chronoTimer;
  struct Timer *eventTimer;
  struct Indicator *error;
  struct Indicator *status;
  struct Interface *can;
  struct ProxyHub *hub;
};
/*----------------------------------------------------------------------------*/
void boardSetup(struct Board *);
void boardStart(struct Board *);
/*----------------------------------------------------------------------------*/
#endif /* BOARD_LPC17XX_DEVKIT_APPLICATION_BOARD_H_ */
