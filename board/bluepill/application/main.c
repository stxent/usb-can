/*
 * board/bluepill/application/main.c
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/generic/work_queue.h>
#include <halm/watchdog.h>
#include <assert.h>
#include <stdlib.h>
/*----------------------------------------------------------------------------*/
static const struct WorkQueueConfig workQueueConfig = {
    .size = 2
};
/*----------------------------------------------------------------------------*/
static void onTimerEventCallback(void *argument)
{
  static const unsigned int EVENT_ITERATIONS = 25;
  static unsigned int iteration = 0;
  static bool phase = true;

  struct Board * const board = argument;
  struct ProxyHub * const hub = board->hub;
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

  indicatorSpin(board->error);

  if (board->wdt != NULL)
    watchdogReload(board->wdt);
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  struct Board * const board = malloc(sizeof(struct Board));

  boardSetupClock();

  /* Initialize Work Queue */
  WQ_DEFAULT = init(WorkQueue, &workQueueConfig);
  assert(WQ_DEFAULT != NULL);

  boardSetup(board);
  timerSetCallback(board->eventTimer, onTimerEventCallback, board);
  boardStart(board);

  /* Start Work Queue */
  wqStart(WQ_DEFAULT);
  return 0;
}
