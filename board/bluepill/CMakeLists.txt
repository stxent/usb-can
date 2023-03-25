# Copyright (C) 2023 xent
# Project is distributed under the terms of the GNU General Public License v3.0

# Set version variables and USB strings
set(VERSION_HW_MAJOR 161 PARENT_SCOPE)
set(VERSION_HW_MINOR 0 PARENT_SCOPE)
set(STRING_VENDOR "Private" PARENT_SCOPE)
set(STRING_PRODUCT "BluePill" PARENT_SCOPE)

# Linker script for an application
configure_file("memory.ld" "${PROJECT_BINARY_DIR}/application.ld")
