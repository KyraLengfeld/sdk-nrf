#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

set(APPLICATION_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../../samples/bluetooth/central_bas)


find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(rom_kpi_test)

target_sources(app PRIVATE ${APPLICATION_SOURCE_DIR}/src/main.c)

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/run_check_rom_size.sh
"#!/bin/sh
export BOARD=${BOARD}
exec \"${PYTHON_EXECUTABLE}\" \"${CMAKE_CURRENT_SOURCE_DIR}/check_rom_size.py\" \"${CMAKE_BINARY_DIR}\"
")
file(CHMOD ${CMAKE_CURRENT_BINARY_DIR}/run_check_rom_size.sh PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

add_custom_target(check_rom_size ALL
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/run_check_rom_size.sh
  DEPENDS rom_report
)
