# Copyright (C) 2018 xent
# Project is distributed under the terms of the GNU General Public License v3.0

if(NOT DEFINED BOARD)
    message(FATAL_ERROR "BOARD not defined")
endif()
if(NOT DEFINED PLATFORM)
    message(FATAL_ERROR "PLATFORM not defined")
endif()

set(FLAGS_LINKER "--specs=nosys.specs --specs=nano.specs -Wl,--gc-sections")

file(GLOB_RECURSE APPLICATION_SOURCES "${BOARD}/application/*.c")
file(GLOB_RECURSE BOOTLOADER_SOURCES "${BOARD}/bootloader/*.c")

add_library(application OBJECT ${APPLICATION_SOURCES})
target_compile_definitions(application PRIVATE -D${PLATFORM})
target_include_directories(application PRIVATE "${BOARD}/application")
target_link_libraries(application PUBLIC halm xcore ${FLAGS_LINKER} -T"${CMAKE_CURRENT_SOURCE_DIR}/${BOARD}/application/memory.ld")

add_library(bootloader OBJECT ${BOOTLOADER_SOURCES})
target_compile_definitions(bootloader PRIVATE -D${PLATFORM})
target_include_directories(bootloader PRIVATE "${BOARD}/bootloader")
target_link_libraries(bootloader PUBLIC halm xcore ${FLAGS_LINKER} -T"${CMAKE_CURRENT_SOURCE_DIR}/${BOARD}/bootloader/memory.ld")
