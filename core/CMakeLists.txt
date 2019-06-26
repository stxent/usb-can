# Copyright (C) 2018 xent
# Project is distributed under the terms of the GNU General Public License v3.0

# Find source files
file(GLOB_RECURSE CORE_SOURCES "*.c")
add_library(core OBJECT ${CORE_SOURCES})
target_compile_definitions(core PRIVATE -D${PLATFORM})
target_include_directories(core PRIVATE ${PROJECT_BINARY_DIR})
target_link_libraries(core PUBLIC halm xcore)
