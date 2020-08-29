# Copyright (C) 2019 xent
# Project is distributed under the terms of the GNU General Public License v3.0

# Set version variables and USB strings
set(VERSION_HW_MAJOR 161 PARENT_SCOPE)
set(VERSION_HW_MINOR 0 PARENT_SCOPE)
set(STRING_VENDOR "Private" PARENT_SCOPE)
set(STRING_PRODUCT "LPC17xx DevKit" PARENT_SCOPE)

# Generate linker scripts
if((DEFINED CMAKE_BUILD_TYPE) AND (${CMAKE_BUILD_TYPE} MATCHES "Release|MinSizeRel"))
    math(EXPR APP_LENGTH "48 * 1024")
    math(EXPR DFU_LENGTH "16 * 1024")
    set(APP_ORIGIN ${DFU_LENGTH})
else()
    math(EXPR APP_LENGTH "192 * 1024")
    math(EXPR DFU_LENGTH "64 * 1024")

    if(${USE_DFU})
      set(APP_ORIGIN ${DFU_LENGTH})
    else()
      set(APP_ORIGIN 0)
    endif()
endif()

# Linker script for a bootloader
set(FLASH_ORIGIN 0)
set(FLASH_LENGTH ${DFU_LENGTH})
configure_file("memory.ld" "${PROJECT_BINARY_DIR}/bootloader.ld")

# Linker script for an application
set(FLASH_ORIGIN ${APP_ORIGIN})
set(FLASH_LENGTH ${APP_LENGTH})
configure_file("memory.ld" "${PROJECT_BINARY_DIR}/application.ld")
