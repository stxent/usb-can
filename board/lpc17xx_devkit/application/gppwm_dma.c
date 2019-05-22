/*
 * gppwm_dma.c
 * Copyright (C) 2019 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <halm/platform/nxp/gpdma_circular.h>
#include <halm/platform/nxp/gpdma_defs.h>
#include <halm/platform/nxp/gpdma_oneshot.h>
#include <halm/platform/nxp/gppwm_defs.h>
#include "gppwm_dma.h"
/*----------------------------------------------------------------------------*/
#define UNPACK_CHANNEL(value)   (((value) >> 4) & 0x0F)
#define UNPACK_FUNCTION(value)  ((value) & 0x0F)
/*----------------------------------------------------------------------------*/
static inline volatile uint32_t *calcMatchChannel(LPC_PWM_Type *, uint8_t);
static uint8_t configMatchPin(uint8_t channel, PinNumber key);
static bool unitAllocateChannel(struct GpPwmDmaUnit *, uint8_t);
static void unitReleaseChannel(struct GpPwmDmaUnit *, uint8_t);
static void unitSetFrequency(struct GpPwmDmaUnit *, uint32_t);
/*----------------------------------------------------------------------------*/
static enum Result unitInit(void *, const void *);
static void unitDeinit(void *);
/*----------------------------------------------------------------------------*/
static enum Result singleEdgeInit(void *, const void *);
static void singleEdgeEnable(void *);
static void singleEdgeDisable(void *);
static uint32_t singleEdgeGetResolution(const void *);
static void singleEdgeSetDuration(void *, uint32_t);
static void singleEdgeSetEdges(void *, uint32_t, uint32_t);
static void singleEdgeSetFrequency(void *, uint32_t);
static void singleEdgeDeinit(void *);
/*----------------------------------------------------------------------------*/
const struct EntityClass * const GpPwmDmaUnit = &(const struct EntityClass){
    .size = sizeof(struct GpPwmDmaUnit),
    .init = unitInit,
    .deinit = unitDeinit
};

const struct PwmClass * const GpPwmDma = &(const struct PwmClass){
    .size = sizeof(struct GpPwmDma),
    .init = singleEdgeInit,
    .deinit = singleEdgeDeinit,

    .enable = singleEdgeEnable,
    .disable = singleEdgeDisable,
    .getResolution = singleEdgeGetResolution,
    .setDuration = singleEdgeSetDuration,
    .setEdges = singleEdgeSetEdges,
    .setFrequency = singleEdgeSetFrequency
};
/*----------------------------------------------------------------------------*/
extern const struct PinEntry gpPwmPins[];
/*----------------------------------------------------------------------------*/
static inline volatile uint32_t *calcMatchChannel(LPC_PWM_Type *device,
    uint8_t channel)
{
  assert(channel && channel <= 6);

  if (channel <= 3)
    return &device->MR1 + (channel - 1);
  else
    return &device->MR4 + (channel - 4);
}
/*----------------------------------------------------------------------------*/
static uint8_t configMatchPin(uint8_t channel, PinNumber key)
{
  const struct PinEntry * const pinEntry = pinFind(gpPwmPins, key, channel);
  assert(pinEntry);

  const struct Pin pin = pinInit(key);

  pinOutput(pin, false);
  pinSetFunction(pin, UNPACK_FUNCTION(pinEntry->value));

  return UNPACK_CHANNEL(pinEntry->value);
}
/*----------------------------------------------------------------------------*/
static bool setupDataChannel(struct GpPwmDma *pwm, uint8_t channel,
    enum GpDmaEvent event)
{
  static const struct GpDmaSettings dmaSettings = {
      .source = {
          .burst = DMA_BURST_4,
          .width = DMA_WIDTH_WORD,
          .increment = true
      },
      .destination = {
          .burst = DMA_BURST_1,
          .width = DMA_WIDTH_WORD,
          .increment = false
      }
  };
  const struct GpDmaOneShotConfig dmaConfig = {
      .event = event,
      .type = GPDMA_TYPE_M2P,
      .channel = channel
  };

  pwm->dma = init(GpDmaOneShot, &dmaConfig);
  if (!pwm->dma)
    return false;
  dmaConfigure(pwm->dma, &dmaSettings);

  return true;
}
/*----------------------------------------------------------------------------*/
static bool setupUpdateChannel(struct GpPwmDmaUnit *unit, uint8_t channel,
    enum GpDmaEvent event)
{
  static const struct GpDmaSettings dmaSettings = {
      .source = {
          .burst = DMA_BURST_4,
          .width = DMA_WIDTH_WORD,
          .increment = false
      },
      .destination = {
          .burst = DMA_BURST_1,
          .width = DMA_WIDTH_WORD,
          .increment = false
      }
  };
  const struct GpDmaCircularConfig dmaConfig = {
      .number = 2,
      .event = event,
      .type = GPDMA_TYPE_M2P,
      .channel = channel,
      .silent = true
  };

  unit->dma = init(GpDmaCircular, &dmaConfig);
  if (!unit->dma)
    return false;
  dmaConfigure(unit->dma, &dmaSettings);

  return true;
}
/*----------------------------------------------------------------------------*/
static bool unitAllocateChannel(struct GpPwmDmaUnit *unit, uint8_t channel)
{
  const uint8_t mask = 1 << channel;

  if (!(unit->matches & mask))
  {
    unit->matches |= mask;
    return true;
  }
  else
    return false;
}
/*----------------------------------------------------------------------------*/
static void unitReleaseChannel(struct GpPwmDmaUnit *unit, uint8_t channel)
{
  unit->matches &= ~(1 << channel);
}
/*----------------------------------------------------------------------------*/
static void unitSetFrequency(struct GpPwmDmaUnit *unit, uint32_t frequency)
{
  LPC_PWM_Type * const reg = unit->base.reg;

  if (frequency)
  {
    const uint32_t apbClock = gpPwmGetClock(&unit->base);
    const uint32_t divisor = apbClock / frequency - 1;

    assert(frequency <= apbClock);

    reg->PR = divisor;
  }
  else
    reg->PR = 0;
}
/*----------------------------------------------------------------------------*/
static enum Result unitInit(void *object, const void *configBase)
{
  const struct GpPwmDmaUnitConfig * const config = configBase;
  assert(config);
  assert(config->resolution >= 2);

  const struct GpPwmUnitBaseConfig baseConfig = {
      .channel = config->channel
  };
  struct GpPwmDmaUnit * const unit = object;
  enum Result res;

  /* Call base class constructor */
  if ((res = GpPwmUnitBase->init(unit, &baseConfig)) != E_OK)
    return res;

  if (!setupUpdateChannel(unit, config->dma, config->event))
    return E_ERROR;

  LPC_PWM_Type * const reg = unit->base.reg;

  reg->TCR = TCR_CRES;

  reg->IR = reg->IR; /* Clear pending interrupts */
  reg->CTCR = 0;
  reg->CCR = 0;
  reg->PCR = 0;

  /* Configure timings */
  unit->frequency = config->frequency;
  unit->resolution = config->resolution;
  unitSetFrequency(unit, unit->frequency * unit->resolution);

  unit->ler = LER_MASK & ~LER_ENABLE(0);
  unit->matches = 0;
  reg->MR0 = unit->resolution;
  reg->MCR = MCR_RESET(0);

  /* Switch to the PWM mode and enable the timer */
  reg->TCR = TCR_CEN | TCR_PWM_ENABLE;

  /* Start periodic transfers that update LER */
  dmaAppend(unit->dma, (void *)&reg->LER, &unit->ler, GPDMA_MAX_TRANSFER);
  dmaAppend(unit->dma, (void *)&reg->LER, &unit->ler, GPDMA_MAX_TRANSFER);
  dmaEnable(unit->dma);

  return E_OK;
}
/*----------------------------------------------------------------------------*/
static void unitDeinit(void *object)
{
  struct GpPwmDmaUnit * const unit = object;
  LPC_PWM_Type * const reg = unit->base.reg;

  dmaDisable(unit->dma);
  reg->TCR = 0;

  GpPwmUnitBase->deinit(unit);
}
/*----------------------------------------------------------------------------*/
static enum Result singleEdgeInit(void *object, const void *configBase)
{
  const struct GpPwmDmaConfig * const config = configBase;
  assert(config);

  struct GpPwmDma * const pwm = object;
  struct GpPwmDmaUnit * const unit = config->parent;

  /* Initialize output pin */
  pwm->channel = configMatchPin(unit->base.channel, config->pin);

  if (!setupDataChannel(pwm, config->dma, config->event))
    return E_ERROR;

  /* Allocate channel */
  if (unitAllocateChannel(unit, pwm->channel))
  {
    LPC_PWM_Type * const reg = unit->base.reg;

    /* Select single edge mode */
    reg->PCR &= ~PCR_DOUBLE_EDGE(pwm->channel);

    pwm->unit = unit;
    pwm->latch = LER_ENABLE(pwm->channel);

    /* Calculate pointer to the match register */
    pwm->value = calcMatchChannel(reg, pwm->channel);

    return E_OK;
  }
  else
    return E_BUSY;
}
/*----------------------------------------------------------------------------*/
static void singleEdgeDeinit(void *object)
{
  struct GpPwmDma * const pwm = object;
  LPC_PWM_Type * const reg = pwm->unit->base.reg;

  reg->PCR &= ~PCR_OUTPUT_ENABLED(pwm->channel);
  unitReleaseChannel(pwm->unit, pwm->channel);
}
/*----------------------------------------------------------------------------*/
static void singleEdgeEnable(void *object)
{
  struct GpPwmDma * const pwm = object;
  LPC_PWM_Type * const reg = pwm->unit->base.reg;

  reg->PCR |= PCR_OUTPUT_ENABLED(pwm->channel);
}
/*----------------------------------------------------------------------------*/
static void singleEdgeDisable(void *object)
{
  struct GpPwmDma * const pwm = object;
  LPC_PWM_Type * const reg = pwm->unit->base.reg;

  reg->PCR &= ~PCR_OUTPUT_ENABLED(pwm->channel);
}
/*----------------------------------------------------------------------------*/
static uint32_t singleEdgeGetResolution(const void *object)
{
  return ((const struct GpPwmDma *)object)->unit->resolution;
}
/*----------------------------------------------------------------------------*/
static void singleEdgeSetDuration(void *object, uint32_t duration)
{
  struct GpPwmDma * const pwm = object;

  /*
   * If match register is set to a value greater than resolution,
   * than output stays high during all cycle.
   */
  *pwm->value = duration;
}
/*----------------------------------------------------------------------------*/
static void singleEdgeSetEdges(void *object,
    uint32_t leading __attribute__((unused)), uint32_t trailing)
{
  assert(leading == 0); /* Leading edge time must be zero */
  singleEdgeSetDuration(object, trailing);
}
/*----------------------------------------------------------------------------*/
static void singleEdgeSetFrequency(void *object, uint32_t frequency)
{
  struct GpPwmDma * const pwm = object;
  struct GpPwmDmaUnit * const unit = pwm->unit;

  unit->frequency = frequency;
  unitSetFrequency(unit, unit->frequency * unit->resolution);
}
/*----------------------------------------------------------------------------*/
/**
 * Create single edge PWM channel.
 * @param unit Pointer to a GpPwmDmaUnit object.
 * @param pin Pin used as a signal output.
 * @param dma DMA channel.
 * @param event DMA event.
 * @return Pointer to a new Pwm object on success or zero on error.
 */
void *gpPwmDmaCreate(void *unit, PinNumber pin, uint8_t dma,
    enum GpDmaEvent event)
{
  const struct GpPwmDmaConfig channelConfig = {
      .parent = unit,
      .pin = pin,
      .dma = dma,
      .event = event
  };

  return init(GpPwmDma, &channelConfig);
}
/*----------------------------------------------------------------------------*/
bool gpPwmDmaIsActive(const void *object)
{
  const struct GpPwmDma * const pwm = object;
  return dmaStatus(pwm->dma) == E_BUSY;
}
/*----------------------------------------------------------------------------*/
void gpPwmDmaSetCallback(void *object, void (*callback)(void *),
    void *argument)
{
  struct GpPwmDma * const pwm = object;
  dmaSetCallback(pwm->dma, callback, argument);
}
/*----------------------------------------------------------------------------*/
void gpPwmDmaStart(void *object, const void *buffer, size_t length)
{
  struct GpPwmDma * const pwm = object;

  dmaDisable(pwm->dma);
  dmaClear(pwm->dma);

  dmaAppend(pwm->dma, (void *)pwm->value, buffer, length);
  dmaEnable(pwm->dma);
}
