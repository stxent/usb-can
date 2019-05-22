/*
 * platform/lpc17xx_devkit/application/main.c
 * Copyright (C) 2018 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/generic/work_queue.h>
#include "board.h"
/*----------------------------------------------------------------------------*/
#define WORK_QUEUE_SIZE  4
#define EVENT_ITERATIONS 25
/*----------------------------------------------------------------------------*/
static struct Board board;
/*----------------------------------------------------------------------------*/
static void onProxyEvent(void *argument, enum ProxyPortMode mode,
    enum ProxyEvent event)
{
  struct SlCanPort * const port = argument;

  if (event == SLCAN_EVENT_RX || event == SLCAN_EVENT_TX)
  {
//    activityIndicatorIncrement(&port->indicator);
    indicatorIncrement(port->status);
  }
  else if (event != SLCAN_EVENT_NONE)
  {
    indicatorIncrement(board.error);
  }

  port->mode = mode;
}
/*----------------------------------------------------------------------------*/
static void onTimerEventCallback(void *argument __attribute__((unused)))
{
  static unsigned int iteration = 0;
  static bool phase = false;
  bool blink = false;

  if (!iteration--)
  {
    timerDisable(board.pwmTimer);

    iteration = EVENT_ITERATIONS;
    phase = !phase;
    blink = true;
  }

  if (board.port0.mode == SLCAN_MODE_DISABLED)
  {
    if (blink)
      indicatorRelax(board.port0.status, phase);
  }
  else
  {
    indicatorSpin(board.port0.status);
  }

  if (board.port1.mode == SLCAN_MODE_DISABLED)
  {
    if (blink)
      indicatorRelax(board.port1.status, phase);
  }
  else
  {
    indicatorSpin(board.port1.status);
  }

//  activityIndicatorSpin(&board.port0.indicator, board.port0.mode, blink);
//  activityIndicatorSpin(&board.port1.indicator, board.port1.mode, blink);
  indicatorSpin(board.error);

  if (blink)
  {
    timerEnable(board.pwmTimer);
  }
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  boardSetupClock();
  boardSetup(&board);

  timerSetCallback(board.eventTimer, onTimerEventCallback, 0);
  proxySetCallback(board.port0.proxy, onProxyEvent, &board.port0);
  proxySetCallback(board.port1.proxy, onProxyEvent, &board.port1);

  boardStart(&board);

  workQueueInit(WORK_QUEUE_SIZE);
  workQueueStart(0);

  return 0;
}
