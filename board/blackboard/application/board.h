/*
 * board/blackboard/application/board.h
 * Copyright (C) 2026 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef BOARD_BLACKBOARD_APPLICATION_BOARD_H_
#define BOARD_BLACKBOARD_APPLICATION_BOARD_H_
/*----------------------------------------------------------------------------*/
#include "board_shared.h"
#include "settings_project.h"
/*----------------------------------------------------------------------------*/
struct ProxyHub;

struct Board
{
  struct Usb *usb;
  struct Interface *can;
  struct Interface *serial;
  struct Timer *chronoTimer;
  struct Timer *eventTimer;
  struct Watchdog *watchdog;

  struct Indicator *error;
  struct Indicator *status;
  struct ProxyHub *hub;

  struct MemoryPackage memoryPackage;
  struct SettingsContext configContext;
  struct Settings config;
  char number[SERIAL_NUMBER_LENGTH];
};
/*----------------------------------------------------------------------------*/
void appBoardInit(struct Board *);
int appBoardStart(struct Board *);
/*----------------------------------------------------------------------------*/
#endif /* BOARD_BLACKBOARD_APPLICATION_BOARD_H_ */
