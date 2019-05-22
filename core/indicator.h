/*
 * core/indicator.h
 * Copyright (C) 2019 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef CORE_INDICATOR_H_
#define CORE_INDICATOR_H_
/*----------------------------------------------------------------------------*/
#include "can_proxy.h"
/*----------------------------------------------------------------------------*/
/* Class descriptor */
struct IndicatorClass
{
  CLASS_HEADER

  void (*increment)(void *);
  void (*relax)(void *, bool);
  void (*spin)(void *);
};

struct Indicator
{
  struct Entity base;
};
/*----------------------------------------------------------------------------*/
BEGIN_DECLS

static inline void indicatorIncrement(void *indicator)
{
  ((const struct IndicatorClass *)CLASS(indicator))->increment(indicator);
}

static inline void indicatorRelax(void *indicator, bool phase)
{
  ((const struct IndicatorClass *)CLASS(indicator))->relax(indicator, phase);
}

static inline void indicatorSpin(void *indicator)
{
  ((const struct IndicatorClass *)CLASS(indicator))->spin(indicator);
}

END_DECLS
/*----------------------------------------------------------------------------*/
#endif /* CORE_INDICATOR_H_ */
