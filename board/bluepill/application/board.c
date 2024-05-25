/*
 * board/bluepill/application/board.c
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include "led_indicator.h"
#include <halm/core/cortex/nvic.h>
#include <halm/core/cortex/systick.h>
#include <halm/generic/lifetime_timer_32.h>
#include <halm/platform/stm32/can.h>
#include <halm/platform/stm32/clocking.h>
#include <halm/platform/stm32/gptimer.h>
#include <halm/platform/stm32/iwdg.h>
#include <halm/platform/stm32/serial_dma.h>
#include <halm/platform/stm32/uart_base.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
/* STM32F1xx has 16 priority levels */

#define PRI_CHRONO  4
#define PRI_CAN     3
/* PRI_DMA, PRI_SERIAL 2 */
#define PRI_TIMER   0

#define EVENT_RATE 50
#define MAX_BLINKS 16
/*----------------------------------------------------------------------------*/
static const struct CanConfig canConfig = {
    .timer = 0,
    .rate = 1000000,
    .rxBuffers = 16,
    /* TX buffer count should be at least SERIALIZED_QUEUE_SIZE */
    .txBuffers = 16,
    .rx = PIN(PORT_B, 8),
    .tx = PIN(PORT_B, 9),
    .priority = PRI_CAN,
    .channel = 0
};

static const struct GpTimerConfig baseTimerConfig = {
    .frequency = 1000000,
    .priority = PRI_CHRONO,
    .channel = TIM2
};

static const struct SysTickConfig eventTimerConfig = {
    .priority = PRI_TIMER
};

static const struct LedIndicatorConfig errorLedConfig = {
    .pin = PIN(PORT_C, 14),
    .limit = MAX_BLINKS,
    .inversion = true
};

static const struct LedIndicatorConfig portLedConfig = {
    .pin = PIN(PORT_C, 13),
    .limit = MAX_BLINKS,
    .inversion = false
};

static const struct SerialDmaConfig serialDmaConfig = {
    .rxChunk = 64,
    .rxLength = 2048,
    .txLength = 1024,
    .rate = 2000000,
    .rx = PIN(PORT_A, 3),
    .tx = PIN(PORT_A, 2),
    .channel = USART2,
    .rxDma = DMA1_STREAM6,
    .txDma = DMA1_STREAM7
};

static const struct IwdgConfig wdtConfig = {
    .period = 1000
};
/*----------------------------------------------------------------------------*/
bool boardSetupClock(void)
{
  static const struct ExternalOscConfig extOscConfig = {
      .frequency = 8000000
  };
  static const struct MainPllConfig mainPllConfig = {
      .divisor = 1,
      .multiplier = 9,
      .source = CLOCK_EXTERNAL
  };
  static const struct SystemClockConfig systemClockConfigPll = {
      .source = CLOCK_PLL
  };

  if (clockEnable(InternalLowSpeedOsc, NULL) != E_OK)
    return false;
  while (!clockReady(InternalLowSpeedOsc));

  if (clockEnable(ExternalOsc, &extOscConfig) != E_OK)
    return false;
  while (!clockReady(ExternalOsc));

  if (clockEnable(MainPll, &mainPllConfig) != E_OK)
    return false;
  while (!clockReady(MainPll));

  clockEnable(Apb1Clock, &(struct BusClockConfig){2});
  clockEnable(Apb2Clock, &(struct BusClockConfig){1});
  clockEnable(MainClock, &(struct BusClockConfig){1});
  clockEnable(SystemClock, &systemClockConfigPll);

  return true;
}
/*----------------------------------------------------------------------------*/
void boardSetup(struct Board *board)
{
#ifdef ENABLE_WDT
  board->watchdog = init(Iwdg, &wdtConfig);
  assert(board->watchdog != NULL);
#else
  (void)wdtConfig;
  board->watchdog = NULL;
#endif

  /* Indication */

  board->error = init(LedIndicator, &errorLedConfig);
  assert(board->error != NULL);
  board->status = init(LedIndicator, &portLedConfig);
  assert(board->status != NULL);

  /* Timers */

  board->baseTimer = init(GpTimer, &baseTimerConfig);
  assert(board->baseTimer != NULL);

  board->eventTimer = init(SysTick, &eventTimerConfig);
  assert(board->eventTimer != NULL);
  timerSetOverflow(board->eventTimer,
      timerGetFrequency(board->eventTimer) / EVENT_RATE);

  const struct LifetimeTimer32Config chronoTimerConfig = {
      .timer = board->baseTimer
  };
  board->chronoTimer = init(LifetimeTimer32, &chronoTimerConfig);
  assert(board->chronoTimer != NULL);

  /* CAN */

  board->can = init(Can, &canConfig);
  assert(board->can != NULL);

  /* Serial */

  board->serial = init(SerialDma, &serialDmaConfig);
  assert(board->serial != NULL);

  /* Create port hub and initialize all ports */

  board->hub = makeProxyHub(1);
  assert(board->hub != NULL);

  const struct ProxyPortConfig proxyPortConfig = {
      .can = board->can,
      .serial = board->serial,
      .chrono = board->chronoTimer,
      .error = board->error,
      .status = board->status,
      .storage = NULL
  };
  proxyPortInit(&board->hub->ports[0], &proxyPortConfig);
}
/*----------------------------------------------------------------------------*/
void boardStart(struct Board *board)
{
  timerEnable(board->chronoTimer);
  timerEnable(board->eventTimer);
}
/*----------------------------------------------------------------------------*/
void resetToBootloader(void)
{
  nvicResetCore();
}
