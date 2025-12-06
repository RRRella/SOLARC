cmake_minimum_required(VERSION 3.28.3)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(OUT_DIR "${CMAKE_BINARY_DIR}/out")
set(BIN_DIR "${OUT_DIR}/$<CONFIG>/bin")
set(LIB_DIR "${OUT_DIR}/$<CONFIG>/lib")
set(DATA_DIR "${CMAKE_SOURCE_DIR}/Data")

project(SOLARC LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SOLARC_BIN_DIR ${CMAKE_BINARY_DIR}/Solarc)
set(SOLARC_CORE_BIN_DIR ${CMAKE_BINARY_DIR}/SolarcCore)
set(SOLARC_TEST_BIN_DIR ${CMAKE_BINARY_DIR}/Test)

add_subdirectory(${CMAKE_SOURCE_DIR}/Code/CommonFlags)

add_subdirectory(${CMAKE_SOURCE_DIR}/Code/SolarcCore              ${SOLARC_CORE_BIN_DIR})
add_subdirectory(${CMAKE_SOURCE_DIR}/Code/Test                    ${SOLARC_TEST_BIN_DIR})
add_subdirectory(${CMAKE_SOURCE_DIR}/Code/Solarc                  ${SOLARC_BIN_DIR})
