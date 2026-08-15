/*
 * Copyright (C) 2024 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

ENTRY(RESET_ISR)
EXTERN(vector_table)

/* Suppress newlib warnings */
EXTERN(__assert_func)

MEMORY
{
  /* Internal flash, Local SRAM or External SRAM */
  ROM (rx)   : ORIGIN = ${ROM_ORIGIN}, LENGTH = ${ROM_LENGTH}
  /* Local SRAM */
  RAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 128K

  OB (rw)    : ORIGIN = 0x1FFEC000, LENGTH = 65K
  APB1 (rw)  : ORIGIN = 0x40000000, LENGTH = 30K
  APB2 (rw)  : ORIGIN = 0x40010000, LENGTH = 19K
  AHB1 (rw)  : ORIGIN = 0x40020000, LENGTH = 384K
  AHB2 (rw)  : ORIGIN = 0x50000000, LENGTH = 387K
  AHB3 (rw)  : ORIGIN = 0xA0000000, LENGTH = 4K
  PPB (rw)   : ORIGIN = 0xE0000000, LENGTH = 256K
  DBG (rw)   : ORIGIN = 0xE0042000, LENGTH = 16
}

SECTIONS
{
  .text : ALIGN(4)
  {
    _stext = .;

    *(.vectors)
    *(.text)
    *(.text.*)
    *(.rodata)
    *(.rodata.*)

    _etext = .;
  } >ROM =0xFF

  .data : ALIGN(4)
  {
    _sdata = .;

    *(.data)
    *(.data.*)

    _edata = .;
  } >RAM AT >ROM

  _sidata = LOADADDR(.data);

  .bss : ALIGN(4)
  {
    _sbss = .;

    *(.bss)
    *(.bss.*)
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

  .ahb1 :
  {
    AHB1_DOMAIN = .;
  } >AHB1

  .ahb2 :
  {
    AHB2_DOMAIN = .;
  } >AHB2

  .ahb3 :
  {
    AHB3_DOMAIN = .;
  } >AHB3

  .ppb :
  {
    PPB_DOMAIN = .;
  } >PPB

  .dbg :
  {
    DBG_DOMAIN = .;
  } >DBG

  /DISCARD/ :
  {
    *(.ARM.exidx*)
    *(.gnu.linkonce.armexidx.*)

    *(.ARM.extab*)
    *(.gnu.linkonce.armextab.*)
  }

  PROVIDE(end = heap_start);
  PROVIDE(_stack = ORIGIN(RAM) + LENGTH(RAM));
}
