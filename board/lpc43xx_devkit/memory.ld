/*
 * Copyright (C) 2018 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

ENTRY(RESET_ISR)
EXTERN(vectorTable)

MEMORY
{
  /* Internal Flash */
  FLASH (rx) : ORIGIN = ${FLASH_ORIGIN}, LENGTH = ${FLASH_LENGTH}
  /* Local SRAM */
  RAM (rwx)  : ORIGIN = 0x10000000, LENGTH = 32K - 32

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

    . = ALIGN(4);
    _sidata = .;
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
  PROVIDE(_stack = ORIGIN(RAM) + LENGTH(RAM));

  PROVIDE(_firmware = LENGTH(FLASH));
  PROVIDE(_seeprom = ORIGIN(EEPROM));
  PROVIDE(_eeeprom = ORIGIN(EEPROM) + LENGTH(EEPROM));
}
