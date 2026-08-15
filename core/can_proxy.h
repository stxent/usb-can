/*
 * core/can_proxy.h
 * Copyright (C) 2018 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef CORE_CAN_PROXY_H_
#define CORE_CAN_PROXY_H_
/*----------------------------------------------------------------------------*/
#include "settings.h"
#include <halm/timer.h>
#include <xcore/interface.h>
/*----------------------------------------------------------------------------*/
extern const struct EntityClass * const CanProxy;

enum [[gnu::packed]] CanProxyEvent
{
  SLCAN_EVENT_NONE,
  SLCAN_EVENT_RX,
  SLCAN_EVENT_TX,
  SLCAN_EVENT_BUS_FAULT,
  SLCAN_EVENT_CAN_OVERRUN,
  SLCAN_EVENT_SERIAL_ERROR,
  SLCAN_EVENT_SERIAL_OVERRUN
};

enum [[gnu::packed]] CanProxyMode
{
  SLCAN_MODE_DISABLED,
  SLCAN_MODE_ACTIVE,
  SLCAN_MODE_LISTENER,
  SLCAN_MODE_LOOPBACK
};

enum [[gnu::packed]] CanProxyNumber
{
  SLCAN_PORT_1,
  SLCAN_PORT_2,
  SLCAN_PORT_3,

  SLCAN_PORT_END
};

struct CanProxy;
typedef void (*CanProxyCallback)(void *, enum CanProxyMode, enum CanProxyEvent);

struct CanProxyConfig
{
  struct Interface *can;
  struct Interface *serial;
  struct Timer *chrono;
  struct SettingsContext *settings;

  CanProxyCallback callback;
  void *argument;

  enum CanProxyNumber number;
};
/*----------------------------------------------------------------------------*/
BEGIN_DECLS

void canProxyChangeMode(struct CanProxy *, enum CanProxyMode);
bool canProxyChangeRate(struct CanProxy *, uint32_t);
uint32_t slcanRatePresetToValue(unsigned int);

END_DECLS
/*----------------------------------------------------------------------------*/
#endif /* CORE_CAN_PROXY_H_ */
