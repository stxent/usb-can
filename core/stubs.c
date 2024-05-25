/*
 * core/stubs.c
 * Copyright (C) 2024 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <xcore/accel.h>
#include <stdio.h>
/*----------------------------------------------------------------------------*/
struct _reent;
/*----------------------------------------------------------------------------*/
void __assert_func(const char *, int, const char *, const char *)
{
  invokeDebugger();
  while (1);
}
/*----------------------------------------------------------------------------*/
void __malloc_lock(struct _reent *)
{
}
/*----------------------------------------------------------------------------*/
void __malloc_unlock(struct _reent *)
{
}
/*----------------------------------------------------------------------------*/
int _close(int)
{
  return -1;
}
/*----------------------------------------------------------------------------*/
off_t _lseek(int, off_t, int)
{
  return 0;
}
/*----------------------------------------------------------------------------*/
int _read(int, char *, int)
{
  return -1;
}
/*----------------------------------------------------------------------------*/
int _write(int, char *, int)
{
  return -1;
}
