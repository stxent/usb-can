/*
 * version.c
 * Copyright (C) 2020 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "version.h"
/*----------------------------------------------------------------------------*/
const struct BoardVersion *getBoardVersion(void)
{
  static const struct BoardVersion ver = {
      {${VERSION_HW_MAJOR}, ${VERSION_HW_MINOR}},
      {${VERSION_SW_MAJOR}, ${VERSION_SW_MINOR}, ${VERSION_SW_REVISION}}
  };
  return &ver;
}
/*----------------------------------------------------------------------------*/
const char *getUsbProductString(void)
{
  static const char str[] = "${STRING_PRODUCT}";
  return str;
}
/*----------------------------------------------------------------------------*/
const char *getUsbVendorString(void)
{
  static const char str[] = "${STRING_VENDOR}";
  return str;
}
