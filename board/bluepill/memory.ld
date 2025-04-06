/*
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

ENTRY(RESET_ISR)
EXTERN(vector_table)

/* Suppress newlib warnings */
EXTERN(__assert_func)

MEMORY
{
  ROM (rx)  : ORIGIN = 0x08000000, LENGTH = 64K
  RAM (rwx) : ORIGIN = 0x20000000, LENGTH = 20K

  OB (rw)   : ORIGIN = 0x1FFFF800, LENGTH = 10
  APB1 (rw) : ORIGIN = 0x40000000, LENGTH = 64K
  APB2 (rw) : ORIGIN = 0x40010000, LENGTH = 32K
  AHB (rw)  : ORIGIN = 0x40018000, LENGTH = 96K
  USB (rw)  : ORIGIN = 0x50000000, LENGTH = 256K
  FSMC (rw) : ORIGIN = 0xA0000000, LENGTH = 256
  PPB (rw)  : ORIGIN = 0xE0000000, LENGTH = 1M
}

SECTIONS
{
  .text : ALIGN(4)
  {
    _stext = .;

    *(.vectors)
    *(.text)
    *(.text*)
    *(.rodata)
    *(.rodata*)

    _etext = .;
  } >ROM =0xFF

  .data : ALIGN(4)
  {
    _sdata = .;

    *(.data)
    *(.data*)

    _edata = .;
  } >RAM AT >ROM

  _sidata = LOADADDR(.data);

  .bss : ALIGN(4)
  {
    _sbss = .;

    *(.bss)
    *(.bss*)
    *(COMMON)

    _ebss = .;

    . = ALIGN(4);
    heap_start = .;
  } >RAM

  .ob :
  {
    OB_DOMAIN = .;
  } >OB

  .apb1 :
  {
    APB1_DOMAIN = .;
  } >APB1

  .apb2 :
  {
    APB2_DOMAIN = .;
  } >APB2

  .ahb :
  {
    AHB_DOMAIN = .;
  } >AHB

  .usb :
  {
    USB_DOMAIN = .;
  } >USB

  .fsmc :
  {
    FSMC_DOMAIN = .;
  } >FSMC

  .ppb :
  {
    PPB_DOMAIN = .;
  } >PPB

  /DISCARD/ :
  {
    *(.init)
    *(.fini)

    *(.ARM.exidx*)
    *(.gnu.linkonce.armexidx.*)

    *(.ARM.extab*)
    *(.gnu.linkonce.armextab.*)
  }

  PROVIDE(end = heap_start);
  PROVIDE(_stack = ORIGIN(RAM) + LENGTH(RAM));
}
