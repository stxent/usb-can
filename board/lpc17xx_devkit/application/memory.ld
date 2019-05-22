/*
 * Copyright (C) 2018 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

ENTRY(RESET_ISR)
EXTERN(vectorTable)

MEMORY
{
  /* Boot debug */
  /*FLASH (rx) : ORIGIN = 0x00010000, LENGTH = 0x020000*/ /* 128 KiB */
  /* 1756 Release */
  /*FLASH (rx) : ORIGIN = 0x00004000, LENGTH = 0x00C000*/ /* 48 KiB */
  /* 1758 tests */
  FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 0x020000 /* 128 KiB */
  /* SRAM */
  /*RAM (rwx)  : ORIGIN = 0x10000000, LENGTH = 0x004000*/ /* 16 KiB */
  /* AHB SRAM0 */
  RAM (rwx)  : ORIGIN = 0x2007C000, LENGTH = 0x004000 /* 16 KiB */

  GPIO (rw)  : ORIGIN = 0x2009C000, LENGTH = 0x004000 /* 16 KiB */
  APB0 (rw)  : ORIGIN = 0x40000000, LENGTH = 0x080000 /* 512 KiB */
  APB1 (rw)  : ORIGIN = 0x40080000, LENGTH = 0x080000 /* 512 KiB */
  AHB (rw)   : ORIGIN = 0x50000000, LENGTH = 0x200000 /* 2 MiB */
  PPB (rw)   : ORIGIN = 0xE0000000, LENGTH = 0x100000 /* 1 MiB */
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

    __preinit_array_start = .;
    KEEP(*(.preinit_array*))
    __preinit_array_end = .;
    __init_array_start = .;
    KEEP(*(.init_array*))
    __init_array_end = .;

    *(.init)
    *(.init*)
    *(.fini)
    *(.fini*)
    *(.ctors)
    *(.ctors*)
    *(.dtors)
    *(.dtors*)
    *(.glue_7)
    *(.glue_7t)
    *(.gnu*)
    *(.gcc*)

    _etext = .;

    . = ALIGN(4);
    _sidata = .;
  } >FLASH =0xFF

  . = ALIGN(4);
  .init_array :
  {
    __init_array_start = .;
    KEEP (*(.init_array*))
    __init_array_end = .;
  } >FLASH =0xFF

  .data : AT(_sidata) ALIGN(4)
  {
    _sdata = .;

    *(.data)
    *(.data*)

    _edata = .;
  } >RAM

  .bss : ALIGN(4)
  {
    _sbss = .;

    *(.bss)
    *(.bss*)
    *(COMMON)

    _ebss = .;

    . = ALIGN(4);
    heapStart = .;
  } >RAM

  .gpio :
  {
    GPIO_DOMAIN = .;
  } >GPIO

  .apb0 :
  {
    APB0_DOMAIN = .;
  } >APB0

  .apb1 :
  {
    APB1_DOMAIN = .;
  } >APB1

  .ahb :
  {
    AHB_DOMAIN = .;
  } >AHB

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

  PROVIDE(end = heapStart);
  PROVIDE(_stack = ORIGIN(RAM) + LENGTH(RAM));
}
