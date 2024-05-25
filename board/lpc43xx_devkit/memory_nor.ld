/*
 * Copyright (C) 2024 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

ENTRY(RESET_ISR)
EXTERN(vectorTable)

/* Suppress newlib warnings */
EXTERN(__assert_func)

MEMORY
{
  ROM (rx)   : ORIGIN = ${ROM_ORIGIN}, LENGTH = ${ROM_LENGTH}
  EEPROM (r) : ORIGIN = 0x20040000, LENGTH = 16K - 128

  AHB (rw)   : ORIGIN = 0x40000000, LENGTH = 72K
  RTC (rw)   : ORIGIN = 0x40040000, LENGTH = 64K
  CLK (rw)   : ORIGIN = 0x40050000, LENGTH = 64K
  APB0 (rw)  : ORIGIN = 0x40080000, LENGTH = 64K
  APB1 (rw)  : ORIGIN = 0x400A0000, LENGTH = 64K
  APB2 (rw)  : ORIGIN = 0x400C0000, LENGTH = 64K
  APB3 (rw)  : ORIGIN = 0x400E0000, LENGTH = 64K
  GPIO (rw)  : ORIGIN = 0x400F4000, LENGTH = 16K
  SPI (rw)   : ORIGIN = 0x40100000, LENGTH = 4K
  SGPIO (rw) : ORIGIN = 0x40101000, LENGTH = 4K
  PPB (rw)   : ORIGIN = 0xE0000000, LENGTH = 1M

  /* Local SRAM */
  SRAM0 (rwx) : ORIGIN = 0x10000000, LENGTH = 128K
  /* Local SRAM */
  SRAM1 (rwx) : ORIGIN = 0x10080000, LENGTH = 72K - 32
  /* AHB SRAM */
  SRAM2 (rwx) : ORIGIN = 0x20000000, LENGTH = 32K
  /* AHB SRAM */
  SRAM3 (rwx) : ORIGIN = 0x20008000, LENGTH = 16K
  /* AHB SRAM */
  SRAM4 (rwx) : ORIGIN = 0x2000C000, LENGTH = 16K
}

SECTIONS
{
  .shared (NOLOAD) : ALIGN(4)
  {
    *(.shared)
    *(.shared*)
  } >SRAM4

  .text : ALIGN(4)
  {
    _stext = .;

    *(.vectors)
    *(.text)
    *(.text*)
    *(.rodata)
    *(.rodata*)

    _etext = .;

    . = ALIGN(4);
    _sidata = .;
  } >ROM =0xFF

  .data : AT(_sidata) ALIGN(4)
  {
    _sdata = .;

    *(.data)
    *(.data*)

    _edata = .;
  } >SRAM1

  .bss : ALIGN(4)
  {
    _sbss = .;

    *(.bss)
    *(.bss*)
    *(COMMON)

    _ebss = .;

    . = ALIGN(4);
    heapStart = .;
  } >SRAM1

  .sram2 (NOLOAD) : ALIGN(4)
  {
    *(.sram2)
    *(.sram2*)
  } >SRAM2

  .sram3 (NOLOAD) : ALIGN(4)
  {
    *(.sram3)
    *(.sram3*)
  } >SRAM3

  .sram4 (NOLOAD) : ALIGN(4)
  {
    *(.sram4)
    *(.sram4*)
  } >SRAM4

  .ahb :
  {
    AHB_DOMAIN = .;
  } >AHB

  .rtc_domain :
  {
    RTC_DOMAIN = .;
  } >RTC

  .clk_domain :
  {
    CLK_DOMAIN = .;
  } >CLK

  .apb0 :
  {
    APB0_DOMAIN = .;
  } >APB0

  .apb1 :
  {
    APB1_DOMAIN = .;
  } >APB1

  .apb2 :
  {
    APB2_DOMAIN = .;
  } >APB2

  .apb3 :
  {
    APB3_DOMAIN = .;
  } >APB3

  .gpio :
  {
    GPIO_DOMAIN = .;
  } >GPIO

  .spi :
  {
    SPI_DOMAIN = .;
  } >SPI

  .sgpio :
  {
    SGPIO_DOMAIN = .;
  } >SGPIO

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
  PROVIDE(_stack = ORIGIN(SRAM1) + LENGTH(SRAM1));

  PROVIDE(_seeprom = ORIGIN(EEPROM));
  PROVIDE(_eeeprom = ORIGIN(EEPROM) + LENGTH(EEPROM));
}
