Installation
------------

USB-CAN project consists of Application and Bootloader firmwares. It requires GNU toolchain for ARM Cortex-M processors, CMake version 3.6 or newer and dfu-util.

Quickstart
----------

Clone git repository:

```sh
git clone https://github.com/stxent/usb-can.git
cd usb-can
git submodule update --init --recursive
```

Build project for Blue Pill Board with STM32F1xx:

```sh
mkdir build
cd build
cmake .. -DPLATFORM=STM32F1XX -DBOARD=bluepill -DCMAKE_TOOLCHAIN_FILE=libs/xcore/toolchains/cortex-m3.cmake -DCMAKE_BUILD_TYPE=Release -DUSE_LTO=ON -DUSE_WDT=ON
make
```

Pins used on STM32F1xx:

```sh
PA2  - UART2 TX
PA3  - UART2 RX
PB8  - CAN1 RXD
PB9  - CAN1 TXD
PC13 - RX/TX LED
PC14 - Error LED
```

Build project for LPC17xx Development Board:

```sh
mkdir build
cd build
cmake .. -DPLATFORM=LPC17XX -DBOARD=lpc17xx_devkit -DCMAKE_TOOLCHAIN_FILE=libs/xcore/toolchains/cortex-m3.cmake -DCMAKE_BUILD_TYPE=Release -DUSE_DFU=ON -DUSE_LTO=ON -DUSE_WDT=ON
make
```

Pins used on LPC17xx:

```sh
P0[0]  - CAN0 RXD
P0[1]  - CAN0 TXD
P0[10] - I2C2 SDA
P0[11] - I2C2 SCL
P0[29] - USB DP
P0[30] - USB DM
P1[9]  - RX/TX LED
P1[10] - Error LED
P1[30] - USB VBUS
P2[9]  - USB CONNECT
```

Build project for LPC43xx Development Board:

```sh
mkdir build
cd build
cmake .. -DPLATFORM=LPC43XX -DBOARD=lpc43xx_devkit -DCMAKE_TOOLCHAIN_FILE=libs/xcore/toolchains/cortex-m4.cmake -DCMAKE_BUILD_TYPE=Release -DUSE_DFU=ON -DUSE_LTO=ON -DUSE_WDT=ON
make
```

Pins used on LPC43xx:

```sh
P3[1] - CAN0 RXD
P3[2] - CAN0 TXD
P2[3] - I2C2 SDA
P2[4] - I2C2 SCL
P5[5] - RX/TX LED
P5[7] - Error LED

USB0 DM
USB0 DP
USB0 VBUS
```

Build project for LPC43xx Development Board with flashless LPC43xx parts:

```sh
mkdir build
cd build
cmake .. -DPLATFORM=LPC43XX -DBOARD=lpc43xx_devkit -DCMAKE_TOOLCHAIN_FILE=libs/xcore/toolchains/cortex-m4.cmake -DCMAKE_BUILD_TYPE=Release -DUSE_DFU=ON -DUSE_LTO=ON -DUSE_NOR=ON -DUSE_WDT=ON
make
```

DFU firmwares are available in the [dpm-examples](https://github.com/stxent/dpm-examples.git) project. The DFU firmware may be flashed using a preferred tool, for example LPC-Link or J-Link. Then an application firmware should be loaded using dfu-util (root access may be required):

```sh
dfu-util -R -D application.bin
```

Useful settings
----------

* CMAKE_BUILD_TYPE — specifies the build type. Possible values are empty, Debug, Release, RelWithDebInfo and MinSizeRel.
* USE_DBG — enables debug messages and profiling.
* USE_DFU — links application firmware using DFU memory layout.
* USE_LTO — enables Link Time Optimization.
* USE_NOR — places application in the NOR Flash instead of the internal Flash.
* USE_WDT — enables Watchdog Timer.
