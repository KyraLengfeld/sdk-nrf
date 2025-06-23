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
