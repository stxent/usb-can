# Copyright (C) 2018 xent
# Project is distributed under the terms of the GNU General Public License v3.0

# Find source files
file(GLOB_RECURSE CORE_SOURCES "*.c")

# Core package
add_library(core ${CORE_SOURCES})
target_include_directories(core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(core PUBLIC dpm)
