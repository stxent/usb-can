/*
 * board/bluepill/application/board.c
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include "led_indicator.h"
#include <halm/core/cortex/nvic.h>
#include <halm/core/cortex/systick.h>
#include <halm/generic/software_timer_32.h>
#include <halm/platform/stm32/can.h>
#include <halm/platform/stm32/clocking.h>
#include <halm/platform/stm32/dma_base.h>
#include <halm/platform/stm32/gptimer.h>
#include <halm/platform/stm32/iwdg.h>
#include <halm/platform/stm32/serial_dma.h>
#include <halm/platform/stm32/uart_base.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
/* STM32F1xx has 16 priority levels */

#define PRI_CHRONO  4
#define PRI_CAN     3
/* PRI_DMA 2 */
#define PRI_SERIAL  2
#define PRI_TIMER   0

#define EVENT_RATE 50
#define MAX_BLINKS 16
/*----------------------------------------------------------------------------*/
static const struct CanConfig canConfig = {
    .timer = 0,
    .rate = 1000000,
    .rxBuffers = 16,
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

static const struct SysTickTimerConfig eventTimerConfig = {
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
    .priority = PRI_SERIAL,
    .channel = USART2,
    .rxDma = DMA1_STREAM6,
    .txDma = DMA1_STREAM7
};

static const struct IwdgConfig wdtConfig = {
    .period = 1000
};
/*----------------------------------------------------------------------------*/
static const struct ExternalOscConfig extOscConfig = {
    .frequency = 8000000
};

static const struct MainPllConfig mainPllConfig = {
    .source = CLOCK_EXTERNAL,
    .divisor = 1,
    .multiplier = 9
};

static const struct BusClockConfig ahbClockConfig = {
    .divisor = 1
};

static const struct BusClockConfig apbClockConfigFast = {
    .divisor = 1
};

static const struct BusClockConfig apbClockConfigSlow = {
    .divisor = 2
};

static const struct SystemClockConfig systemClockConfigPll = {
    .source = CLOCK_PLL
};
/*----------------------------------------------------------------------------*/
void boardSetupClock(void)
{
  enum Result res;

  res = clockEnable(InternalLowSpeedOsc, 0);
  assert(res == E_OK);
  while (!clockReady(InternalLowSpeedOsc));

  res = clockEnable(ExternalOsc, &extOscConfig);
  assert(res == E_OK);
  while (!clockReady(ExternalOsc));

  res = clockEnable(MainPll, &mainPllConfig);
  assert(res == E_OK);
  while (!clockReady(MainPll));

  clockEnable(Apb1Clock, &apbClockConfigSlow);
  clockEnable(Apb2Clock, &apbClockConfigFast);
  clockEnable(MainClock, &ahbClockConfig);
  clockEnable(SystemClock, &systemClockConfigPll);

  /* Suppress warning */
  (void)res;
}
/*----------------------------------------------------------------------------*/
void boardSetup(struct Board *board)
{
#ifdef ENABLE_WDT
  board->wdt = init(Iwdg, &wdtConfig);
  assert(board->wdt);
#else
  (void)wdtConfig;
  board->wdt = 0;
#endif

  /* Indication */

  board->error = init(LedIndicator, &errorLedConfig);
  assert(board->error);
  board->status = init(LedIndicator, &portLedConfig);
  assert(board->status);

  /* Timers */

  board->baseTimer = init(GpTimer, &baseTimerConfig);
  assert(board->baseTimer);

  board->eventTimer = init(SysTickTimer, &eventTimerConfig);
  assert(board->eventTimer);
  timerSetOverflow(board->eventTimer,
      timerGetFrequency(board->eventTimer) / EVENT_RATE);

  const struct SoftwareTimer32Config chronoTimerConfig = {
      .timer = board->baseTimer
  };
  board->chronoTimer = init(SoftwareTimer32, &chronoTimerConfig);
  assert(board->chronoTimer);

  /* CAN */

  board->can = init(Can, &canConfig);
  assert(board->can);

  /* Serial */

  board->serial = init(SerialDma, &serialDmaConfig);
  assert(board->serial);

  /* Create port hub and initialize all ports */

  board->hub = makeProxyHub(1);
  assert(board->hub);

  const struct ProxyPortConfig proxyPortConfig = {
      .can = board->can,
      .serial = board->serial,
      .chrono = board->chronoTimer,
      .error = board->error,
      .status = board->status,
      .storage = 0
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
