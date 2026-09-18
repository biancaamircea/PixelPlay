
# Copyright 2026 NXP
#
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.22.0)

cmake_path(APPEND conf_file_path ${CMAKE_CURRENT_LIST_DIR} ${board} prj.conf)
set(CONF_FILE ${conf_file_path} ${CONF_FILE})
include(${SdkRootDirPath}/cmake/extension/mcux.cmake)

project(new LANGUAGES C CXX ASM PROJECT_BOARD_PORT_PATH ${CMAKE_CURRENT_LIST_DIR}/${board})

include(${SdkRootDirPath}/CMakeLists.txt)

mcux_project_remove_source(
    BASE_PATH ${SdkRootDirPath}/components/misc_utilities
    SOURCES fsl_memcpy.S
            fsl_memset.S
)

mcux_add_armgcc_linker_script(
    TARGETS debug release
    BASE_PATH ${SdkRootDirPath}
    LINKER devices/MCX/MCXA/MCXA153/gcc/MCXA153_flash.ld
)

include(${board}/board_files.cmake)

mcux_add_source(
    SOURCES led_blinky.c
            app/board_ili9341.c
            app/ili9341.c
            app/ili9341_demo.c
            app/uart.c
)

mcux_convert_binary(BINARY ${APPLICATION_BINARY_DIR}/${MCUX_SDK_PROJECT_NAME}.bin)
mcux_add_config_mex_path( PATH ${board} )
mcux_add_include(INCLUDES .)
