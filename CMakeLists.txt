cmake_minimum_required(VERSION 3.5)
project(c22-fpga)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

set(FPGA_PART xcu280-fsvh2892-2L-e)

# RoCE/U280 stack requires 512-bit (64-byte) data path. Force this BEFORE
# add_subdirectory so per-submodule `set(DATA_WIDTH 8 CACHE ...)` defaults
# (e.g. fpga-network-stack/hls/ip_handler/CMakeLists.txt:25) cannot win.
set(DATA_WIDTH 64 CACHE STRING "Width of data path in bytes" FORCE)

if (DEFINED ENV{IPREPO_DIR})
	set(IPREPO_DIR $ENV{IPREPO_DIR})
elseif(NOT  IPREPO_DIR)
	set(IPREPO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/build/iprepo)
endif()

find_package(Vitis REQUIRED)
if (NOT VITIS_HLS_FOUND)
	message(FATAL_ERROR "Vitis HLS not found.")
endif()

add_subdirectory(krnl)
add_subdirectory(fpga-network-stack)
