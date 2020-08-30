/*
 * board/lpc17xx_devkit/application/main.c
 * Copyright (C) 2018 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/generic/work_queue.h>
#include "board.h"
#include "board_shared.h"
/*----------------------------------------------------------------------------*/
#define WORK_QUEUE_SIZE  2
#define EVENT_ITERATIONS 25
/*----------------------------------------------------------------------------*/
static struct Board board;
/*----------------------------------------------------------------------------*/
static void onTimerEventCallback(void *argument)
{
  static unsigned int iteration = 0;
  static bool phase = true;

  struct ProxyHub * const hub = argument;
  bool blink = false;

  if (!iteration--)
  {
    iteration = EVENT_ITERATIONS;
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

  indicatorSpin(board.error);
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  boardSetupClock();
  boardSetup(&board);
  timerSetCallback(board.eventTimer, onTimerEventCallback, board.hub);
  boardStart(&board);

  workQueueInit(WORK_QUEUE_SIZE);
  workQueueStart(0);

  return 0;
}
