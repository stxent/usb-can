/*
 * board/lpc17xx_devkit/board.h
 * Copyright (C) 2019 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef BOARD_LPC17XX_DEVKIT_BOARD_H_
#define BOARD_LPC17XX_DEVKIT_BOARD_H_
/*----------------------------------------------------------------------------*/
#include "indicator.h"
/*----------------------------------------------------------------------------*/
struct Entity;
struct Interface;
struct CanProxy;
struct Timer;
/*----------------------------------------------------------------------------*/
struct SlCanPort
{
  struct Interface *can;
  struct Interface *serial;
  struct CanProxy *proxy;
  enum ProxyPortMode mode;
  struct Indicator *status;
};

struct Board
{
  struct Entity *usb;
  struct Entity *composite;
  struct Timer *chronoTimer;
  struct Timer *eventTimer;
  struct Timer *pwmTimer;
  struct Timer *pwmUnitTimer;
  struct GpPwmDmaUnit *pwmUnit;
  struct GpPwmDma *pwm0;
  struct GpPwmDma *pwm1;
  struct Indicator *error;
  struct SlCanPort port0;
  struct SlCanPort port1;
};
/*----------------------------------------------------------------------------*/
BEGIN_DECLS

void boardSetupClock(void);
void boardSetup(struct Board *);
void boardStart(struct Board *);

END_DECLS
/*----------------------------------------------------------------------------*/
#endif /* BOARD_LPC17XX_DEVKIT_BOARD_H_ */
