# Copyright (C) 2018 xent
# Project is distributed under the terms of the GNU General Public License v3.0

project(USB2CAN C)
cmake_minimum_required(VERSION 3.13)
cmake_policy(SET CMP0054 NEW)
cmake_policy(SET CMP0069 NEW)

option(USE_DFU "Use memory layout for the bootloader." OFF)
option(USE_LTO "Enable Link Time Optimization." OFF)
option(USE_WDT "Enable Watchdog Timer." OFF)

# Default compiler flags
set(FLAGS_DEFAULT "-fdata-sections -ffunction-sections -Wall -Wextra -Wshadow -pedantic")
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLAGS_DEFAULT} ${FLAGS_CPU}")
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")

if(${USE_LTO})
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
        set(CMAKE_C_COMPILE_OPTIONS_IPO -flto -ffat-lto-objects)
    endif()
endif()

# Configure XCORE library
add_subdirectory(libs/xcore xcore)

# Configure HALM library, HALM_CONFIG_FILE should be defined
set(HALM_CONFIG_FILE "${PROJECT_SOURCE_DIR}/board/${BOARD}/halm.config" CACHE INTERNAL "Path to the library configuration file" FORCE)
add_subdirectory(libs/halm halm)

# Configure DPM library
add_subdirectory(libs/dpm dpm)

add_subdirectory(core)
add_subdirectory(board)

add_executable("application.elf" $<TARGET_OBJECTS:application>)
target_link_libraries("application.elf" PRIVATE application)
add_custom_command(TARGET "application.elf"
        POST_BUILD
        COMMAND "${CMAKE_OBJCOPY}" "application.elf" -Obinary "application.bin"
)

if(TARGET bootloader)
    add_executable("bootloader.elf" $<TARGET_OBJECTS:bootloader>)
    target_link_libraries("bootloader.elf" PRIVATE bootloader)
    add_custom_command(TARGET "bootloader.elf"
            POST_BUILD
            COMMAND "${CMAKE_OBJCOPY}" "bootloader.elf" -Oihex "bootloader.hex"
    )
endif()
