#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(NONE)

include_directories(
	${ZEPHYR_NRF_MODULE_DIR}/samples/bluetooth/mesh/common
)

# copy_directory function
function(copy_directory src_dir dest_dir)
    if(IS_DIRECTORY ${src_dir})
        # Create destination dir
        file(MAKE_DIRECTORY ${dest_dir})

        # Get the list of files and dirs to copy
        file(GLOB_RECURSE files_to_copy ${src_dir}/*)
        foreach(file ${files_to_copy})
            if(NOT IS_DIRECTORY ${file})
                # Calc the relative path for the file
                file(RELATIVE_PATH relative_file ${src_dir} ${file})
                set(dest_file ${dest_dir}/${relative_file})

                # Create dir structure
                get_filename_component(dest_dir_path ${dest_file} DIRECTORY)
                file(MAKE_DIRECTORY ${dest_dir_path})

                zephyr_file_copy(${file} ${dest_file})
            endif()
        endforeach()
    else()
        message(FATAL_ERROR "Source directory does not exist: ${src_dir}")
    endif()
endfunction()

# Note: this happens too late during compilation for it to affect the mcuboot part!
copy_directory(${ZEPHYR_NRF_MODULE_DIR}/samples/bluetooth/mesh/common/mcuboot ${CMAKE_CURRENT_SOURCE_DIR}/sysbuild/mcuboot)

# NORDIC SDK APP START
target_sources(app PRIVATE
	src/main.c
	src/model_handler.c)
target_include_directories(app PRIVATE
	include
	${ZEPHYR_NRF_MODULE_DIR}/samples/bluetooth/mesh/common
)

# Preinitialization related to DFU over SMP
target_sources_ifdef(CONFIG_MCUMGR_TRANSPORT_BT app PRIVATE
	${ZEPHYR_NRF_MODULE_DIR}/samples/bluetooth/mesh/common/smp_bt.c)

# NORDIC SDK APP END
