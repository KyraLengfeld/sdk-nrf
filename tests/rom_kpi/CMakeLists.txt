#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# No C code needed, we are only using this to let Twister build something
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(rom_kpi_test)

# zephyr_include_directories(.)

target_sources(app PRIVATE main.c)

add_custom_command(TARGET app POST_BUILD
  COMMAND
    ${PYTHON_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/test_rom_kpi.py
    $<TARGET_FILE:app>
    ${ZEPHYR_BINARY_DIR}
    ${BOARD}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Running ROM KPI test."
)
