/*
 * core/version.h
 * Copyright (C) 2020 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef CORE_VERSION_H_
#define CORE_VERSION_H_
/*----------------------------------------------------------------------------*/
#include <xcore/helpers.h>
#include <stdint.h>
/*----------------------------------------------------------------------------*/
struct BoardVersion
{
  struct
  {
    uint16_t major;
    uint16_t minor;
  } hw;

  struct
  {
    uint16_t major;
    uint16_t minor;
    uint32_t revision;
    uint32_t hash;
  } sw;

  uint32_t timestamp;
};
/*----------------------------------------------------------------------------*/
BEGIN_DECLS

const struct BoardVersion *getBoardVersion(void);
const char *getBuildHostname(void);
const char *getUsbProductString(void);
const char *getUsbVendorString(void);

END_DECLS
/*----------------------------------------------------------------------------*/
#endif /* CORE_VERSION_H_ */
