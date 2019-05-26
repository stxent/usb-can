/*
 * core/proxy_port.h
 * Copyright (C) 2019 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef CORE_PROXY_PORT_H_
#define CORE_PROXY_PORT_H_
/*----------------------------------------------------------------------------*/
#include "can_proxy.h"
#include "indicator.h"
/*----------------------------------------------------------------------------*/
struct Indicator;

struct ProxyPortConfig
{
  struct Interface *can;
  struct Interface *serial;
  struct Timer *chrono;
  struct Indicator *error;
  struct Indicator *status;
};

struct ProxyPort
{
  struct CanProxy *proxy;
  struct Indicator *error;
  struct Indicator *status;

  struct
  {
    enum CanProxyMode current;
    enum CanProxyMode next;
  } mode;
};

struct ProxyHub
{
  size_t size;
  struct ProxyPort ports[];
};
/*----------------------------------------------------------------------------*/
struct ProxyHub *makeProxyHub(size_t);
void proxyPortInit(struct ProxyPort *, const struct ProxyPortConfig *);
/*----------------------------------------------------------------------------*/
#endif /* CORE_PROXY_PORT_H_ */
