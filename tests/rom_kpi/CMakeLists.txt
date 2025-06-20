#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# No C code needed, we are only using this to let Twister build something
cmake_minimum_required(VERSION 3.20.0)
# # set(BOARD nrf5340dk/nrf5340/cpuapp)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(rom_kpi_test)

# zephyr_include_directories(.)
target_sources(app PRIVATE main.c)

# Sample path to be built but build it in test dir
set(SAMPLE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../samples/bluetooth/central_bas)
set(SAMPLE_BUILD_DIR ${CMAKE_BINARY_DIR}/central_bas_build)

# main custom target: builds, runs rom_report, and runs python script
add_custom_target(test_rom_kpi ALL
    #Configure + build sample
    COMMAND ${CMAKE_COMMAND} -B ${SAMPLE_BUILD_DIR} -S ${SAMPLE_DIR} -GNinja -DBOARD=$ENV{BOARD}
    COMMAND ${CMAKE_COMMAND} --build ${SAMPLE_BUILD_DIR}

    # Run rom_report for the sample
    COMMAND ${CMAKE_COMMAND} --build ${SAMPLE_BUILD_DIR} --target rom_report

    # Run python script
    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_rom_kpi.py

    COMMENT "Running ROM KPI test."
)


# # # # target_sources(app PRIVATE main.c)

# # # # add_custom_command(TARGET app POST_BUILD
# # # #   COMMAND
# # # #     ${PYTHON_EXECUTABLE}
# # # #     ${CMAKE_CURRENT_SOURCE_DIR}/test_rom_kpi.py
# # # #     $<TARGET_FILE:app>
# # # #     ${ZEPHYR_BINARY_DIR}
# # # #     ${BOARD}
# # # #   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
# # # #   COMMENT "Running ROM KPI test."
# # # # )



# # # # set(sample_dir ${CMAKE_CURRENT_LIST_DIR}/../../../../samples/bluetooth/peripheral_hids_mouse)
# # # # set(CONF_FILE ${sample_dir}/prj.conf)

# # # # add_subdirectory(${sample_dir} sample)

# # # # add_custom_target(check_ram_size ALL
# # # #   ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/check_ram_size.py ${CMAKE_BINARY_DIR}
# # # #   DEPENDS ram_report
# # # # )


# # # # --------------------
