# Copyright (C) 2019 xent
# Project is distributed under the terms of the GNU General Public License v3.0

# Set version variables and USB strings
set(VERSION_HW_MAJOR 161 PARENT_SCOPE)
set(VERSION_HW_MINOR 0 PARENT_SCOPE)
set(STRING_VENDOR "Private" PARENT_SCOPE)
set(STRING_PRODUCT "LPC17xx DevKit" PARENT_SCOPE)

set(FLASH_BASE 0)

# Generate linker scripts
if((DEFINED CMAKE_BUILD_TYPE) AND (${CMAKE_BUILD_TYPE} MATCHES "Release|MinSizeRel"))
    math(EXPR FLASH_SIZE "64 * 1024")
    math(EXPR DFU_LENGTH "16 * 1024")
else()
    math(EXPR FLASH_SIZE "256 * 1024")
    math(EXPR DFU_LENGTH "64 * 1024")
endif()

# Generate linker script
if(USE_DFU)
    math(EXPR ROM_LENGTH "${FLASH_SIZE} - ${DFU_LENGTH}")
    math(EXPR ROM_ORIGIN "${FLASH_BASE} + ${DFU_LENGTH}")
else()
    math(EXPR ROM_LENGTH "${FLASH_SIZE}")
    math(EXPR ROM_ORIGIN "${FLASH_BASE}")
endif()

# Linker script for a main application and tests
configure_file("memory.ld" "${PROJECT_BINARY_DIR}/memory.ld")
