# Copyright (C) 2018 xent
# Project is distributed under the terms of the GNU General Public License v3.0

project(USB2CAN C)
cmake_minimum_required(VERSION 3.6)
cmake_policy(SET CMP0054 NEW)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "The type of build: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

option(USE_DFU "Use memory layout for the bootloader." OFF)
option(USE_LTO "Enable Link Time Optimization." OFF)

# Configure build flags
set(FLAGS_DEFAULT "-fdata-sections -ffunction-sections -Wall -Wextra -Wshadow -pedantic")

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLAGS_DEFAULT} ${FLAGS_CPU}")
# set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")

if(${USE_LTO})
    # TODO Use standard option
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto -ffat-lto-objects")
endif()

# Configure XCORE library
set(BITWISE_CRC ON)
add_subdirectory(libs/xcore xcore)

# Configure HALM library, CONFIG_FILE should be defined
set(CONFIG_FILE "${PROJECT_SOURCE_DIR}/board/${BOARD}/halm.config" CACHE INTERNAL "Path to the library configuration file" FORCE)
add_subdirectory(libs/halm halm)

add_subdirectory(core)
add_subdirectory(board)

# set(CMAKE_EXECUTABLE_SUFFIX ".elf")
add_executable("application.elf" $<TARGET_OBJECTS:core> $<TARGET_OBJECTS:application>)
target_include_directories(application PRIVATE core)
target_link_libraries("application.elf" PRIVATE core application)
add_custom_command(TARGET "application.elf"
        POST_BUILD
        COMMAND "${CMAKE_OBJCOPY}" "application.elf" -Obinary "application.bin"
)

add_executable("bootloader.elf" $<TARGET_OBJECTS:core> $<TARGET_OBJECTS:bootloader>)
target_include_directories(bootloader PRIVATE core)
target_link_libraries("bootloader.elf" PRIVATE core bootloader)
add_custom_command(TARGET "bootloader.elf"
        POST_BUILD
        COMMAND "${CMAKE_OBJCOPY}" "bootloader.elf" -Oihex "bootloader.hex"
)
