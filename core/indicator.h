/*
 * core/indicator.h
 * Copyright (C) 2019 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef CORE_INDICATOR_H_
#define CORE_INDICATOR_H_
/*----------------------------------------------------------------------------*/
#include <stdbool.h>
#include "can_proxy.h"
/*----------------------------------------------------------------------------*/
/* Class descriptor */
struct IndicatorClass
{
  CLASS_HEADER

  void (*add)(void *, unsigned int);
  void (*relax)(void *, bool);
  void (*spin)(void *);
};

struct Indicator
{
  struct Entity base;
};
/*----------------------------------------------------------------------------*/
static inline void indicatorAdd(void *indicator, unsigned int value)
{
  ((const struct IndicatorClass *)CLASS(indicator))->add(indicator, value);
}

static inline void indicatorRelax(void *indicator, bool phase)
{
  ((const struct IndicatorClass *)CLASS(indicator))->relax(indicator, phase);
}

static inline void indicatorSpin(void *indicator)
{
  ((const struct IndicatorClass *)CLASS(indicator))->spin(indicator);
}
/*----------------------------------------------------------------------------*/
#endif /* CORE_INDICATOR_H_ */
