# Copyright (C) 2018 xent
# Project is distributed under the terms of the GNU General Public License v3.0

if(NOT DEFINED BOARD)
    message(FATAL_ERROR "BOARD not defined")
endif()
if(NOT DEFINED PLATFORM)
    message(FATAL_ERROR "PLATFORM not defined")
endif()

add_subdirectory("${BOARD}")
set(FLAGS_LINKER "--specs=nosys.specs --specs=nano.specs -Wl,--gc-sections")

file(GLOB_RECURSE APPLICATION_SOURCES "${BOARD}/application/*.c")
file(GLOB_RECURSE BOOTLOADER_SOURCES "${BOARD}/bootloader/*.c")
file(GLOB_RECURSE SHARED_SOURCES "${BOARD}/shared/*.c")

# Main application
add_library(application OBJECT ${APPLICATION_SOURCES} ${SHARED_SOURCES})
target_compile_definitions(application PRIVATE -D${PLATFORM})
target_include_directories(application PRIVATE "${BOARD}/application" "${BOARD}/shared")
target_link_libraries(application PUBLIC halm xcore ${FLAGS_LINKER} -T"${PROJECT_BINARY_DIR}/application.ld")

# Bootloader
add_library(bootloader OBJECT ${BOOTLOADER_SOURCES} ${SHARED_SOURCES})
target_compile_definitions(bootloader PRIVATE -D${PLATFORM})
target_include_directories(bootloader PRIVATE "${BOARD}/bootloader" "${BOARD}/shared")
target_link_libraries(bootloader PUBLIC halm xcore ${FLAGS_LINKER} -T"${PROJECT_BINARY_DIR}/bootloader.ld")
