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

# Generate version file
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include(GetGitRevisionDescription)

git_describe(_version_sw_tag --abbrev=0 --tags)
git_commit_count(VERSION_SW_REVISION)
string(REPLACE "." ";" _version_sw_tag_splitted "${_version_sw_tag}")
list(GET _version_sw_tag_splitted 0 VERSION_SW_MAJOR)
list(GET _version_sw_tag_splitted 1 VERSION_SW_MINOR)
configure_file("version_template.c" "${PROJECT_BINARY_DIR}/version.c")

# Find source files
file(GLOB_RECURSE APPLICATION_SOURCES "${BOARD}/application/*.c")
file(GLOB_RECURSE BOOTLOADER_SOURCES "${BOARD}/bootloader/*.c")
file(GLOB_RECURSE SHARED_SOURCES "${BOARD}/shared/*.c")
list(APPEND SHARED_SOURCES "${PROJECT_BINARY_DIR}/version.c")

# Shared package
add_library(shared ${SHARED_SOURCES})
target_include_directories(shared PUBLIC "${BOARD}/shared")
# Shared library depends on includes from core library
target_link_libraries(shared PUBLIC core)
# Core library depends on version functions from shared library
target_link_libraries(core PRIVATE shared)

# Main application package
add_library(application OBJECT ${APPLICATION_SOURCES})
target_include_directories(application PRIVATE "${BOARD}/application")
target_link_libraries(application PRIVATE shared ${FLAGS_LINKER} -T"${PROJECT_BINARY_DIR}/application.ld")

if(USE_WDT)
    target_compile_definitions(application PRIVATE -DENABLE_WDT)
endif()

# Bootloader package
if(BOOTLOADER_SOURCES)
    add_library(bootloader OBJECT ${BOOTLOADER_SOURCES})
    target_include_directories(bootloader PRIVATE "${BOARD}/bootloader")
    target_link_libraries(bootloader PRIVATE shared ${FLAGS_LINKER} -T"${PROJECT_BINARY_DIR}/bootloader.ld")
endif()
