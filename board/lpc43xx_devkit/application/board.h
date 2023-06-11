/*
 * board/lpc43xx_devkit/application/board.h
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef BOARD_LPC43XX_DEVKIT_APPLICATION_BOARD_H_
#define BOARD_LPC43XX_DEVKIT_APPLICATION_BOARD_H_
/*----------------------------------------------------------------------------*/
#include "param_storage.h"
#include "proxy_port.h"
/*----------------------------------------------------------------------------*/
struct Interface;
struct Timer;
struct Watchdog;

struct Board
{
  struct Entity *usb;
  struct Interface *can;
  struct Interface *i2c;
  struct Interface *eeprom;
  struct Interface *serial;
  struct Timer *chronoTimer;
  struct Timer *eepromTimer;
  struct Timer *eventTimer;
  struct Watchdog *watchdog;

  struct Indicator *error;
  struct Indicator *status;
  struct ProxyHub *hub;

  struct ParamStorage storage;
  struct SerialNumber number;
};
/*----------------------------------------------------------------------------*/
void appBoardInit(struct Board *);
int appBoardStart(struct Board *);
/*----------------------------------------------------------------------------*/
#endif /* BOARD_LPC43XX_DEVKIT_APPLICATION_BOARD_H_ */
