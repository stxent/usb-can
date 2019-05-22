/*
 * board/lpc17xx_devkit/gppwm_dma.h
 * Copyright (C) 2019 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef BOARD_LPC17XX_DEVKIT_GPPWM_DMA_H_
#define BOARD_LPC17XX_DEVKIT_GPPWM_DMA_H_
/*----------------------------------------------------------------------------*/
#include <halm/dma.h>
#include <halm/platform/nxp/gpdma_base.h>
#include <halm/platform/nxp/gppwm_base.h>
#include <halm/pwm.h>
/*----------------------------------------------------------------------------*/
extern const struct EntityClass * const GpPwmDmaUnit;

struct GpPwmDmaUnitConfig
{
  /** Mandatory: switching frequency. */
  uint32_t frequency;
  /** Mandatory: cycle resolution. */
  uint32_t resolution;
  /** Mandatory: peripheral identifier. */
  uint8_t channel;
  /** Mandatory: DMA channel for LER transfers. */
  uint8_t dma;
  /** Mandatory: DMA event for LER transfers. */
  enum GpDmaEvent event;
};

struct GpPwmDmaUnit
{
  struct GpPwmUnitBase base;

  /* DMA channel descriptor */
  struct Dma *dma;
  /* LER value to be loaded periodically */
  uint32_t ler;

  /* Desired timer frequency */
  uint32_t frequency;
  /* Cycle width measured in timer ticks */
  uint32_t resolution;
  /* Match blocks currently in use */
  uint8_t matches;
};
/*----------------------------------------------------------------------------*/
extern const struct PwmClass * const GpPwmDma;

struct GpPwmDmaConfig
{
  /** Mandatory: peripheral unit. */
  struct GpPwmDmaUnit *parent;
  /** Mandatory: pin used as an output for modulated signal. */
  PinNumber pin;
  /** Mandatory: DMA channel. */
  uint8_t dma;
  /** Mandatory: DMA event. */
  enum GpDmaEvent event;
};

struct GpPwmDma
{
  struct Pwm base;

  /* Pointer to a parent unit */
  struct GpPwmDmaUnit *unit;
  /* DMA channel descriptor */
  struct Dma *dma;
  /* Pointer to a match register */
  volatile uint32_t *value;
  /* Match channel number */
  uint8_t channel;
  /* Mask for the Latch Enable Register */
  uint8_t latch;
};
/*----------------------------------------------------------------------------*/
BEGIN_DECLS

void *gpPwmDmaCreate(void *, PinNumber, uint8_t, enum GpDmaEvent);
bool gpPwmDmaIsActive(const void *);
void gpPwmDmaSetCallback(void *, void (*)(void *), void *);
void gpPwmDmaStart(void *, const void *, size_t);

END_DECLS
/*----------------------------------------------------------------------------*/
#endif /* BOARD_LPC17XX_DEVKIT_GPPWM_DMA_H_ */
