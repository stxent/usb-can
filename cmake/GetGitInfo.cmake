# Copyright (C) 2023 xent
# Project is distributed under the terms of the GNU General Public License v3.0

function(git_commit_count _sw_commit_count)
    if(NOT GIT_FOUND)
        find_package(Git QUIET)
    endif()

    if(GIT_FOUND)
        execute_process(COMMAND
                "${GIT_EXECUTABLE}" rev-list HEAD --count
                WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                RESULT_VARIABLE _res
                OUTPUT_VARIABLE _out
                ERROR_QUIET
                OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        if(_res EQUAL 0)
            set(${_sw_commit_count} "${_out}" PARENT_SCOPE)
            return()
        endif()
    endif()

    set(${_sw_commit_count} 0 PARENT_SCOPE)
endfunction()

function(git_hash _sw_hash)
    if(NOT GIT_FOUND)
        find_package(Git QUIET)
    endif()

    if(GIT_FOUND)
        execute_process(COMMAND
                "${GIT_EXECUTABLE}" rev-parse --short=8 HEAD
                WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                RESULT_VARIABLE _res
                OUTPUT_VARIABLE _out
                ERROR_QUIET
                OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        if(${_res} EQUAL 0)
            set(${_sw_hash} "${_out}" PARENT_SCOPE)
            return()
        endif()
    endif()

    set(${_sw_hash} "00000000" PARENT_SCOPE)
endfunction()

function(git_version _sw_major _sw_minor)
    if(NOT GIT_FOUND)
        find_package(Git QUIET)
    endif()

    if(GIT_FOUND)
        execute_process(COMMAND
                "${GIT_EXECUTABLE}" describe --tags --abbrev=0
                WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                RESULT_VARIABLE _res
                OUTPUT_VARIABLE _out
                ERROR_QUIET
                OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        if(${_res} EQUAL 0)
            string(REPLACE "." ";" _sw_version ${_out})
            list(LENGTH _sw_version _sw_version_length)

            if(${_sw_version_length} EQUAL 2)
                list(GET _sw_version 0 _sw_version_major)
                list(GET _sw_version 1 _sw_version_minor)

                set(${_sw_major} ${_sw_version_major} PARENT_SCOPE)
                set(${_sw_minor} ${_sw_version_minor} PARENT_SCOPE)
                return()
            endif()
        endif()
    endif()

    set(${_sw_major} 0 PARENT_SCOPE)
    set(${_sw_minor} 0 PARENT_SCOPE)
endfunction()
