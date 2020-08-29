/*
 * core/version.h
 * Copyright (C) 2020 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef CORE_VERSION_H_
#define CORE_VERSION_H_
/*----------------------------------------------------------------------------*/
#include <stdint.h>
#include <xcore/helpers.h>
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
  } sw;
};
/*----------------------------------------------------------------------------*/
BEGIN_DECLS

const struct BoardVersion *getBoardVersion(void);
const char *getUsbProductString(void);
const char *getUsbVendorString(void);

END_DECLS
/*----------------------------------------------------------------------------*/
#endif /* CORE_VERSION_H_ */
