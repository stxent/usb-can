/*
 * version.h
 * Copyright (C) 2019 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef VERSION_H_
#define VERSION_H_
/*----------------------------------------------------------------------------*/
#define VERSION_HW_MAJOR ${VERSION_HW_MAJOR}
#define VERSION_HW_MINOR ${VERSION_HW_MINOR}
#define VERSION_SW_MAJOR ${VERSION_SW_MAJOR}
#define VERSION_SW_MINOR ${VERSION_SW_MINOR}

static inline const char *usbProductString(void)
{
  static const char str[] = "${STRING_PRODUCT}";
  return str;
}

static inline const char *usbVendorString(void)
{
  static const char str[] = "${STRING_VENDOR}";
  return str;
}
/*----------------------------------------------------------------------------*/
#endif /* VERSION_H_ */
