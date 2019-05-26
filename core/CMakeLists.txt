# Copyright (C) 2018 xent
# Project is distributed under the terms of the GNU General Public License v3.0

# Generate version file
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
include(GetGitRevisionDescription)

set(VERSION_HW_MAJOR 1)
set(VERSION_HW_MINOR 0)
git_describe(_version_sw_tag --abbrev=0 --tags)
string(REPLACE "." ";" _version_sw_tag_splitted "${_version_sw_tag}")
list(GET _version_sw_tag_splitted 0 VERSION_SW_MAJOR)
list(GET _version_sw_tag_splitted 1 VERSION_SW_MINOR)
configure_file("version_template.h" "${PROJECT_BINARY_DIR}/version.h")

file(GLOB_RECURSE CORE_SOURCES "*.c")
add_library(core OBJECT ${CORE_SOURCES})
target_compile_definitions(core PRIVATE -D${PLATFORM})
target_include_directories(core PRIVATE ${PROJECT_BINARY_DIR})
# target_include_directories(core INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}") # TODO
target_link_libraries(core PUBLIC halm xcore)
