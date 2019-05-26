/*
 * core/proxy_port.c
 * Copyright (C) 2019 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <stdlib.h>
#include "indicator.h"
#include "proxy_port.h"
/*----------------------------------------------------------------------------*/
static void onProxyEvent(void *, enum CanProxyMode, enum CanProxyEvent);
/*----------------------------------------------------------------------------*/
static void onProxyEvent(void *object, enum CanProxyMode mode,
    enum CanProxyEvent event)
{
  struct ProxyPort * const port = object;

  if (event == SLCAN_EVENT_RX || event == SLCAN_EVENT_TX)
    indicatorAdd(port->status, 2);
  else if (event != SLCAN_EVENT_NONE)
    indicatorAdd(port->error, 2);

  if (port->mode.next != mode && mode != SLCAN_MODE_DISABLED)
    indicatorAdd(port->status, 1);

  port->mode.next = mode;
}
/*----------------------------------------------------------------------------*/
struct ProxyHub *makeProxyHub(size_t size)
{
  struct ProxyHub * const hub = malloc(sizeof(struct ProxyHub)
      + size * sizeof(struct ProxyPort));

  if (hub)
    hub->size = size;

  return hub;
}
/*----------------------------------------------------------------------------*/
void proxyPortInit(struct ProxyPort *port, const struct ProxyPortConfig *config)
{
  const struct CanProxyConfig proxyConfig = {
      .can = config->can,
      .serial = config->serial,
      .chrono = config->chrono
  };
  struct CanProxy * const proxy = init(CanProxy, &proxyConfig);
  assert(proxy);

  port->proxy = proxy;
  port->error = config->error;
  port->status = config->status;
  port->mode.current = SLCAN_MODE_DISABLED;
  port->mode.next = SLCAN_MODE_DISABLED;

  proxySetCallback(proxy, onProxyEvent, port);
}
