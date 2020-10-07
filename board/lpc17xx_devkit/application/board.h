/*
 * board/lpc17xx_devkit/application/board.h
 * Copyright (C) 2019 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef BOARD_LPC17XX_DEVKIT_APPLICATION_BOARD_H_
#define BOARD_LPC17XX_DEVKIT_APPLICATION_BOARD_H_
/*----------------------------------------------------------------------------*/
#include "param_storage.h"
#include "proxy_port.h"
#include <halm/timer.h>
#include <xcore/interface.h>
/*----------------------------------------------------------------------------*/
struct Board
{
  struct Entity *usb;
  struct Interface *i2c;
  struct Interface *eeprom;
  struct Interface *serial;
  struct Timer *chronoTimer;
  struct Timer *eventTimer;
  struct Indicator *error;
  struct Indicator *status;
  struct Interface *can;
  struct ProxyHub *hub;

  struct ParamStorage storage;
  struct SerialNumber number;
};
/*----------------------------------------------------------------------------*/
void boardSetup(struct Board *);
void boardStart(struct Board *);
/*----------------------------------------------------------------------------*/
#endif /* BOARD_LPC17XX_DEVKIT_APPLICATION_BOARD_H_ */
