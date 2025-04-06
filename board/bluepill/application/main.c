/*
 * board/bluepill/application/main.c
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/watchdog.h>
#include <stdlib.h>
/*----------------------------------------------------------------------------*/
static void onTimerEventCallback(void *argument)
{
  static const unsigned int eventRateDecimation = 25;
  static unsigned int iteration = 0;
  static bool phase = true;

  struct Board * const board = argument;
  struct ProxyHub * const hub = board->hub;
  bool blink = false;

  if (!iteration--)
  {
    iteration = eventRateDecimation;
    phase = !phase;
    blink = true;
  }

  for (size_t i = 0; i < hub->size; ++i)
  {
    struct ProxyPort * const port = &hub->ports[i];

    if (port->mode.current != port->mode.next && blink && phase)
      port->mode.current = port->mode.next;

    if (port->mode.current == SLCAN_MODE_DISABLED)
    {
      if (blink)
        indicatorRelax(port->status, phase);
    }
    else
      indicatorSpin(port->status);
  }

  indicatorSpin(board->error);

  if (board->watchdog != NULL)
    watchdogReload(board->watchdog);
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  struct Board * const board = malloc(sizeof(struct Board));

  boardSetupClock();
  boardSetup(board);

  timerSetCallback(board->eventTimer, onTimerEventCallback, board);
  return boardStart(board);
}
