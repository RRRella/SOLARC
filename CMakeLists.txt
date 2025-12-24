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

# ============================================================================
# Renderer Backend Configuration (Global)
# ============================================================================

# This allows the renderer selection to be visible across all subdirectories
# Default to DX12 on Windows, Vulkan on Linux
if(WIN32)
    set(SOLARC_RENDERER "DX12" CACHE STRING "Graphics API backend")
elseif(UNIX AND NOT APPLE)
    set(SOLARC_RENDERER "Vulkan" CACHE STRING "Graphics API backend")
endif()

message(STATUS "==============================================")
message(STATUS "Solarc Engine Build Configuration")
message(STATUS "==============================================")
message(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Renderer: ${SOLARC_RENDERER}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "==============================================")

# ============================================================================
# Global Build Definitions
# ============================================================================

add_compile_definitions(
    $<$<CONFIG:Debug>:SOLARC_DEBUG_BUILD>
    $<$<CONFIG:Release>:SOLARC_RELEASE_BUILD>
    # Optional: Treat RelWithDebInfo as a Release build if you ever use it
    $<$<CONFIG:RelWithDebInfo>:SOLARC_RELEASE_BUILD>
    $<$<CONFIG:MinSizeRel>:SOLARC_RELEASE_BUILD>
)

# ============================================================================
# Subdirectories
# ============================================================================


set(SOLARC_BIN_DIR ${CMAKE_BINARY_DIR}/Solarc)
set(SOLARC_CORE_BIN_DIR ${CMAKE_BINARY_DIR}/SolarcCore)
set(SOLARC_TEST_BIN_DIR ${CMAKE_BINARY_DIR}/Test)

add_subdirectory(${CMAKE_SOURCE_DIR}/Code/CommonFlags)

add_subdirectory(${CMAKE_SOURCE_DIR}/Code/SolarcCore              ${SOLARC_CORE_BIN_DIR})
add_subdirectory(${CMAKE_SOURCE_DIR}/Code/Test                    ${SOLARC_TEST_BIN_DIR})
add_subdirectory(${CMAKE_SOURCE_DIR}/Code/Solarc                  ${SOLARC_BIN_DIR})
