# USB-CAN

![C23](https://img.shields.io/badge/C-23-blue)
![CMake](https://img.shields.io/badge/CMake-3.21+-blue)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

An open-source firmware implementation for LPC and STM32 microcontrollers
that establishes a high-performance bridge between CAN 2.0 A/B
and the SLCAN protocol.

## Overview

Depending on the targeted hardware platform, the firmware natively supports
multiple CAN ports. On boards equipped with physical USB hardware, it enumerates
as a USB composite device, exposing a dedicated virtual COM port for each
individual CAN interface. For hardware architectures lacking native USB support,
the communication is implemented using a standard UART. Non-volatile I²C EEPROM
is used to store configuration settings.

The project is written in C (C23 standard) and uses CMake for the build system.

## Supported Platforms and Features

### Key Features

* Supports 11-bit and 29-bit CAN identifiers.
* Supports Remote Transmission Request (RTR) messages.
* Optional blocking USB mode to ensure lossless read/write operations.
* Capable of lossless operation at CAN speeds up to 1 Mbit/s.

### Supported Boards

* STM32 Blue Pill (STM32F1xx)
* STM32 Black Board (STM32F4xx)
* LPC17xx development board
* LPC43xx development board (including flashless variants)

## Required Packages

To build and use the USB-CAN project, the following software components
are required:

* **ARM GCC 13 or newer** — Arm GNU Toolchain, required for Cortex-M
  embedded targets.
* **CMake 3.21 or newer** — used for configuring and generating build systems
  across platforms.
* **Kconfiglib** or **kconfig-frontends** — tools for interactive Halm library
  configuration via `menuconfig`.
* **dfu-util** — required for flashing firmware via USB bootloader.

## Pinouts

### STM32F1xx Blue Pill Board

| Pin  | Function  |
|------|-----------|
| PA2  | UART2 TX  |
| PA3  | UART2 RX  |
| PB8  | CAN1 RXD  |
| PB9  | CAN1 TXD  |
| PC13 | RX/TX LED |
| PC14 | Error LED |

### STM32F4xx Black Board

| Pin  | Function  |
|------|-----------|
| PA11 | USB FS DM |
| PA12 | USB FS DP |
| PB6  | I2C1 SCL  |
| PB7  | I2C1 SDA  |
| PB8  | CAN1 RXD  |
| PB9  | CAN1 TXD  |
| PF9  | RX/TX LED |
| PF10 | Error LED |

### LPC17xx Development Board

| Pin    | Function    |
|--------|-------------|
| P0[0]  | CAN0 RXD    |
| P0[1]  | CAN0 TXD    |
| P0[10] | I2C2 SDA    |
| P0[11] | I2C2 SCL    |
| P0[29] | USB DP      |
| P0[30] | USB DM      |
| P1[9]  | RX/TX LED   |
| P1[10] | Error LED   |
| P1[30] | USB VBUS    |
| P2[9]  | USB CONNECT |

### LPC43xx Development Board

| Pin   | Function  |
|-------|-----------|
| P2[3] | I2C1 SDA  |
| P2[4] | I2C1 SCL  |
| P3[1] | CAN0 RXD  |
| P3[2] | CAN0 TXD  |
| P5[5] | RX/TX LED |
| P5[7] | Error LED |
| -     | USB0 DM   |
| -     | USB0 DP   |
| -     | USB0 VBUS |

## Getting Started

Clone the repository and initialize submodules:

```sh
git clone https://github.com/stxent/usb-can.git
cd usb-can
git submodule update --init --recursive
```

## Build Firmwares

### Build for STM32F1xx Blue Pill

```sh
mkdir build && cd build
cmake .. -DPLATFORM=STM32F1XX -DBOARD=bluepill -DCMAKE_TOOLCHAIN_FILE=libs/xcore/toolchains/cortex-m3.cmake -DCMAKE_BUILD_TYPE=Release -DUSE_LTO=ON -DUSE_WDT=ON
make
```

### Build for STM32F4xx Black Board

```sh
mkdir build && cd build
cmake .. -DPLATFORM=STM32F4XX -DBOARD=blackboard -DCMAKE_TOOLCHAIN_FILE=libs/xcore/toolchains/cortex-m4.cmake -DCMAKE_BUILD_TYPE=Release -DUSE_DFU=ON -DUSE_LTO=ON -DUSE_WDT=ON
make
```

### Build for LPC17xx Development Board

```sh
mkdir build && cd build
cmake .. -DPLATFORM=LPC17XX -DBOARD=lpc17xx_devkit -DCMAKE_TOOLCHAIN_FILE=libs/xcore/toolchains/cortex-m3.cmake -DCMAKE_BUILD_TYPE=Release -DUSE_DFU=ON -DUSE_LTO=ON -DUSE_WDT=ON
make
```

### Build for LPC43xx Development Board

```sh
mkdir build && cd build
cmake .. -DPLATFORM=LPC43XX -DBOARD=lpc43xx_devkit -DCMAKE_TOOLCHAIN_FILE=libs/xcore/toolchains/cortex-m4.cmake -DCMAKE_BUILD_TYPE=Release -DUSE_DFU=ON -DUSE_LTO=ON -DUSE_WDT=ON
make
```

### Build for Flashless LPC43xx Parts

```sh
mkdir build && cd build
cmake .. -DPLATFORM=LPC43XX -DBOARD=lpc43xx_devkit -DCMAKE_TOOLCHAIN_FILE=libs/xcore/toolchains/cortex-m4.cmake -DCMAKE_BUILD_TYPE=Release -DUSE_DFU=ON -DUSE_NOR=ON -DUSE_WDT=ON
make
```

### Build Artifacts and Deployment

After successful compilation, the firmware files are organized as follows:

* All firmware files are stored in the board directory within
  the build directory.
* The main application firmware is located in `application.bin`.
* DFU firmwares are available in the
  [dpm-examples](https://github.com/stxent/dpm-examples.git) project.

### Flashing Instructions

The generated firmware can be flashed using various tools:

* **LPC-Link** programmer
* **J-Link** programmer
* **dfu-util** for DFU-enabled builds (root access may be required)

To flash the device, follow these steps:

* Flash the DFU firmware using tools such as LPC-Link or J-Link.
* Load the application firmware using `dfu-util`:
  ```sh
  dfu-util -R -D application.bin
  ```

## Build Options

The following build options are available to customize your build:

* **CMAKE_BUILD_TYPE** — Specifies the build type. Possible values:
  empty, `Debug`, `Release`, `RelWithDebInfo`, `MinSizeRel`.
* **USE_DBG** — Enables debug messages.
* **USE_DFU** — Links the application firmware using the DFU memory layout.
* **USE_LTO** — Enables Link Time Optimization.
* **USE_NOR** — Places the application in NOR Flash instead of internal Flash.
* **USE_WDT** — Activates Watchdog Timer functionality.

## SLCAN Commands

The firmware supports the following SLCAN commands for controlling the bridge
and querying device information:

| Command | Description |
| --- | --- |
| `Sx` | Set standard speed variant `x` (from speed table) |
| `sxxxx` | Set custom baud rate (4-6 hex chars) |
| `tiiildd..` | Transmit standard (11-bit) frame |
| `Tiiiiiiiildd..` | Transmit extended (29-bit) frame |
| `riiil` | Transmit standard RTR (11-bit) frame |
| `Riiiiiiiil` | Transmit extended RTR (29-bit) frame |
| `O` | Open port |
| `L` | Enter listener mode |
| `l` | Enter loopback mode |
| `C` | Close port |
| `V` | Request board version (returns 4 hex bytes) |
| `v` | Request firmware version (returns 4 hex bytes) |
| `N` | Request serial number (returns 4 hex bytes) |
| `Nxxxx` | Set serial number (one-time operation, hex) |
| `Ax` | Toggle automatic retransmission (`0` = disable, `1` = enable) |
| `B` | Reboot into bootloader mode |
| `bx` | Toggle blocking mode (`0` = disable, `1` = enable) |
| `F` | Query flags (legacy, returns `0000`) |
| `I` | Query initial speed, returns default variant or `F` if disabled |
| `Ix` | Set initial speed to variant `x` (from speed table) or `F` to disable |
| `W` | Legacy command (unused) |
| `x` | Generate test message sequence |
| `Zx` | Toggle timestamp generation (`0` = disable, `1` = enable) |

Possible speed variants:

| Value | Speed     |
|-------|-----------|
| 0     | 10 kbaud  |
| 1     | 20 kbaud  |
| 2     | 50 kbaud  |
| 3     | 100 kbaud |
| 4     | 125 kbaud |
| 5     | 250 kbaud |
| 6     | 500 kbaud |
| 7     | 800 kbaud |
| 8     | 1 Mbaud   |
