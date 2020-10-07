/*
 * core/can_proxy.h
 * Copyright (C) 2018 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef CORE_CAN_PROXY_H_
#define CORE_CAN_PROXY_H_
/*----------------------------------------------------------------------------*/
#include "param_storage.h"
#include <halm/timer.h>
#include <xcore/interface.h>
/*----------------------------------------------------------------------------*/
extern const struct EntityClass * const CanProxy;

enum CanProxyEvent
{
  SLCAN_EVENT_NONE,
  SLCAN_EVENT_RX,
  SLCAN_EVENT_TX,
  SLCAN_EVENT_BUS_FAULT,
  SLCAN_EVENT_CAN_OVERRUN,
  SLCAN_EVENT_SERIAL_ERROR,
  SLCAN_EVENT_SERIAL_OVERRUN
};

enum CanProxyMode
{
  SLCAN_MODE_DISABLED,
  SLCAN_MODE_ACTIVE,
  SLCAN_MODE_LISTENER,
  SLCAN_MODE_LOOPBACK
} __attribute__((packed));

typedef void (*ProxyCallback)(void *, enum CanProxyMode, enum CanProxyEvent);

struct CanProxy;

struct CanProxyConfig
{
  struct Interface *can;
  struct Interface *serial;
  struct Timer *chrono;
  struct ParamStorage *storage;
};
/*----------------------------------------------------------------------------*/
BEGIN_DECLS

void proxySetCallback(struct CanProxy *, ProxyCallback, void *);

END_DECLS
/*----------------------------------------------------------------------------*/
#endif /* CORE_CAN_PROXY_H_ */
