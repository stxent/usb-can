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

Build project for LPC17xx Development Board:

```sh
mkdir build
cd build
cmake .. -DPLATFORM=LPC17XX -DBOARD=lpc17xx_devkit -DCMAKE_TOOLCHAIN_FILE=libs/xcore/toolchains/cortex-m3.cmake -DCMAKE_BUILD_TYPE=Release -DUSE_LTO=ON
make
```

The Bootloader firmware is located in bootloader.hex file and may be flashed using a preferred tool, for example LPC-Link or J-Link. Then the Application firmware must be loaded using dfu-util (root access may be required):

```sh
dfu-util -R -D application.bin
```

Useful settings
----------

* CMAKE_BUILD_TYPE — specifies the build type. Possible values are empty, Debug, Release, RelWithDebInfo and MinSizeRel.
* USE_DFU — links application firmware using DFU memory layout even in Debug and RelWithDebInfo modes.
* USE_LTO — enables Link Time Optimization.
