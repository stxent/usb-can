/*
 * core/proxy_port.c
 * Copyright (C) 2019 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "indicator.h"
#include "proxy_port.h"
#include <assert.h>
#include <stdlib.h>
/*----------------------------------------------------------------------------*/
static void onProxyEvent(void *, enum CanProxyMode, enum CanProxyEvent);
/*----------------------------------------------------------------------------*/
static void onProxyEvent(void *object, enum CanProxyMode mode,
    enum CanProxyEvent event)
{
  struct ProxyPort * const port = object;

  if (port->error)
  {
    if (event >= SLCAN_EVENT_BUS_FAULT && mode != SLCAN_MODE_DISABLED)
      indicatorIncrement(port->error);
  }

  if (port->status)
  {
    if (event == SLCAN_EVENT_RX || event == SLCAN_EVENT_TX)
      indicatorIncrement(port->status);
  }

  if (port->mode.next != mode)
  {
    if (port->status && mode != SLCAN_MODE_DISABLED)
      indicatorSet(port->status, 1);

    port->mode.next = mode;
  }
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
void proxyPortDeinit(struct ProxyPort *port)
{
  deinit(port->proxy);
}
/*----------------------------------------------------------------------------*/
void proxyPortInitTemplate(struct ProxyPort *port,
    const struct ProxyPortConfig *config, const struct EntityClass *type)
{
  const struct CanProxyConfig proxyConfig = {
      .can = config->can,
      .serial = config->serial,
      .chrono = config->chrono,
      .storage = config->storage,
      .callback = onProxyEvent,
      .argument = port
  };

  port->error = config->error;
  port->status = config->status;
  port->mode.current = SLCAN_MODE_DISABLED;
  port->mode.next = SLCAN_MODE_DISABLED;

  port->proxy = init(type, &proxyConfig);
  assert(port->proxy);
}
/*----------------------------------------------------------------------------*/
void proxyPortInit(struct ProxyPort *port, const struct ProxyPortConfig *config)
{
  proxyPortInitTemplate(port, config, CanProxy);
}
