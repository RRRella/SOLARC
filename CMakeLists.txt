cmake_minimum_required(VERSION 3.30.2)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(OUT_DIR "${CMAKE_BINARY_DIR}/out")
set(BIN_DIR "${OUT_DIR}/$<CONFIG>/bin")
set(LIB_DIR "${OUT_DIR}/$<CONFIG>/lib")
set(DATA_DIR "${CMAKE_SOURCE_DIR}/Data")

project(EnhancedNerf LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include("${CMAKE_SOURCE_DIR}/cmake/SETUP_CONFIGURATIONS.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/DEFAULT_DEFS.cmake")

set(ENERF_BIN_DIR ${CMAKE_BINARY_DIR}/ENerf)
set(ENERF_CORE_BIN_DIR ${CMAKE_BINARY_DIR}/ENerfCore)
set(ENERF_TEST_BIN_DIR ${CMAKE_BINARY_DIR}/Test)

add_subdirectory(${CMAKE_SOURCE_DIR}/code/ENerfCore              ${ENERF_CORE_BIN_DIR})
add_subdirectory(${CMAKE_SOURCE_DIR}/code/Test                   ${ENERF_TEST_BIN_DIR})
add_subdirectory(${CMAKE_SOURCE_DIR}/code/ENerf                  ${ENERF_BIN_DIR})