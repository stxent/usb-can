/*
 * board/bluepill/application/board.h
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef BOARD_BLUEPILL_APPLICATION_BOARD_H_
#define BOARD_BLUEPILL_APPLICATION_BOARD_H_
/*----------------------------------------------------------------------------*/
#include "proxy_port.h"
/*----------------------------------------------------------------------------*/
struct Interface;
struct Timer;
struct Watchdog;

struct Board
{
  struct Interface *can;
  struct Interface *serial;
  struct Timer *baseTimer;
  struct Timer *chronoTimer;
  struct Timer *eventTimer;
  struct Watchdog *watchdog;

  struct Indicator *error;
  struct Indicator *status;
  struct ProxyHub *hub;
};
/*----------------------------------------------------------------------------*/
bool boardSetupClock(void);
void boardSetup(struct Board *);
void boardStart(struct Board *);
/*----------------------------------------------------------------------------*/
#endif /* BOARD_BLUEPILL_APPLICATION_BOARD_H_ */
