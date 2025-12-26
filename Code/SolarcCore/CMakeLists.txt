cmake_minimum_required(VERSION 3.22)

project(SOLARC_CORE LANGUAGES CXX)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/SRC_FILES.cmake")

add_library(${PROJECT_NAME} SHARED ${${PROJECT_NAME}_SRC} ${${PROJECT_NAME}_HDRS})

include("${CMAKE_SOURCE_DIR}/cmake/GROUP_FILES.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/DEFS.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/PCH.cmake")

#spdlog setup
set(SPDLOG_BIN_DIR ${CMAKE_BINARY_DIR}/spdlog)
option(SPDLOG_BUILD_SHARED "Build spdlog as a shared library" OFF)
add_subdirectory(${CMAKE_SOURCE_DIR}/ThirdParty/spdlog ${SPDLOG_BIN_DIR})
target_include_directories(${PROJECT_NAME}
    PUBLIC ${CMAKE_SOURCE_DIR}/ThirdParty/spdlog/include
)
set_target_properties( spdlog
     PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${BIN_DIR}"
     LIBRARY_OUTPUT_DIRECTORY "${LIB_DIR}"
     ARCHIVE_OUTPUT_DIRECTORY "${LIB_DIR}"
)
#

#nlohmann json
set(NLOHMANN_JSON_BIN_DIR ${CMAKE_BINARY_DIR}/nlohmann-json)
add_subdirectory(${CMAKE_SOURCE_DIR}/ThirdParty/nlohmann-json    ${NLOHMANN_JSON_BIN_DIR})
target_include_directories(${PROJECT_NAME}
    PUBLIC ${CMAKE_SOURCE_DIR}/ThirdParty/nlohmann-json/single_include
)
#

#toml11
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/ThirdParty/toml11/include)
#

# Platform-specific libraries
if(UNIX AND NOT APPLE)

    enable_language(C)
    
    # Find Wayland
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(WAYLAND REQUIRED wayland-client)
    pkg_check_modules(XDG_SHELL REQUIRED wayland-protocols)
    
    # Generate xdg-shell protocol
    pkg_get_variable(WAYLAND_PROTOCOLS_DIR wayland-protocols pkgdatadir)
    pkg_get_variable(WAYLAND_SCANNER wayland-scanner wayland_scanner)
    
    set(XDG_SHELL_PROTOCOL "${WAYLAND_PROTOCOLS_DIR}/stable/xdg-shell/xdg-shell.xml")
    set(XDG_SHELL_CLIENT_HEADER "${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-client-protocol.h")
    set(XDG_SHELL_CODE "${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-protocol.c")
    
    add_custom_command(
        OUTPUT "${XDG_SHELL_CLIENT_HEADER}"
        COMMAND ${WAYLAND_SCANNER} client-header "${XDG_SHELL_PROTOCOL}" "${XDG_SHELL_CLIENT_HEADER}"
        DEPENDS "${XDG_SHELL_PROTOCOL}"
        VERBATIM
    )
    
    add_custom_command(
        OUTPUT "${XDG_SHELL_CODE}"
        COMMAND ${WAYLAND_SCANNER} private-code "${XDG_SHELL_PROTOCOL}" "${XDG_SHELL_CODE}"
        DEPENDS "${XDG_SHELL_PROTOCOL}"
        VERBATIM
    )

    set(XDG_DECORATION_PROTOCOL "${WAYLAND_PROTOCOLS_DIR}/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml")
    set(XDG_DECORATION_CLIENT_HEADER "${CMAKE_CURRENT_BINARY_DIR}/xdg-decoration-unstable-v1-client-protocol.h")
    set(XDG_DECORATION_CODE "${CMAKE_CURRENT_BINARY_DIR}/xdg-decoration-unstable-v1-protocol.c")
    
    add_custom_command(
        OUTPUT "${XDG_DECORATION_CLIENT_HEADER}"
        COMMAND ${WAYLAND_SCANNER} client-header "${XDG_DECORATION_PROTOCOL}" "${XDG_DECORATION_CLIENT_HEADER}"
        DEPENDS "${XDG_DECORATION_PROTOCOL}"
        VERBATIM
    )
    
    add_custom_command(
        OUTPUT "${XDG_DECORATION_CODE}"
        COMMAND ${WAYLAND_SCANNER} private-code "${XDG_DECORATION_PROTOCOL}" "${XDG_DECORATION_CODE}"
        DEPENDS "${XDG_DECORATION_PROTOCOL}"
        VERBATIM
    )
    
    # Add generated files to sources
    target_sources(${PROJECT_NAME} PRIVATE
        ${XDG_SHELL_CLIENT_HEADER}
        ${XDG_SHELL_CODE}
        ${XDG_DECORATION_CLIENT_HEADER}
        ${XDG_DECORATION_CODE}
    )
    
    target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
    target_include_directories(${PROJECT_NAME} PUBLIC ${WAYLAND_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${WAYLAND_LIBRARIES})
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${${PROJECT_NAME}_SRC_DIR})
target_include_directories(${PROJECT_NAME} PUBLIC ${${PROJECT_NAME}_INC_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog COMMON_FLAGS)


# ============================================================================
# Renderer Backend Selection
# ============================================================================

# Renderer selection variable (user-configurable)
set(SOLARC_RENDERER "DX12" CACHE STRING "Graphics API backend (DX12 or VULKAN)")
set_property(CACHE SOLARC_RENDERER PROPERTY STRINGS "DX12" "VULKAN")

message(STATUS "Selected renderer backend: ${SOLARC_RENDERER}")

# Platform validation
if(WIN32)
    if(SOLARC_RENDERER STREQUAL "DX12")
        message(STATUS "Building with DirectX 12 backend on Windows")
    elseif(SOLARC_RENDERER STREQUAL "VULKAN")
        message(STATUS "Building with VULKAN backend on Windows")
    else()
        message(FATAL_ERROR "Invalid renderer: ${SOLARC_RENDERER}. Must be DX12 or VULKAN")
    endif()
elseif(UNIX AND NOT APPLE)
    if(SOLARC_RENDERER STREQUAL "VULKAN")
        message(STATUS "Building with VULKAN backend on Linux")
    elseif(SOLARC_RENDERER STREQUAL "DX12")
        message(FATAL_ERROR "DirectX 12 is not supported on Linux. Use VULKAN.")
    else()
        message(FATAL_ERROR "Invalid renderer: ${SOLARC_RENDERER}. Must be VULKAN on Linux")
    endif()
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

# Set renderer preprocessor definition
target_compile_definitions(${PROJECT_NAME} PUBLIC SOLARC_RENDERER_${SOLARC_RENDERER})

# ============================================================================
# Renderer-Specific Libraries and Dependencies
# ============================================================================

if(SOLARC_RENDERER STREQUAL "DX12")
    # DirectX 12 libraries (Windows SDK - already installed)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        d3d12.lib
        dxgi.lib
        dxguid.lib
    )
    message(STATUS "Linked DirectX 12 libraries: d3d12, dxgi, dxguid")

elseif(SOLARC_RENDERER STREQUAL "VULKAN")
    # VULKAN SDK
    find_package(Vulkan REQUIRED)
    
    if(Vulkan_FOUND)
        target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})
        target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan)
        message(STATUS "Found VULKAN SDK: ${Vulkan_LIBRARY}")
    else()
        message(FATAL_ERROR "VULKAN SDK not found. Please install VULKAN SDK.")
    endif()
endif()

set_target_properties( ${PROJECT_NAME}
     PROPERTIES
     RUNTIME_OUTPUT_DIRECTORY "${BIN_DIR}"
     LIBRARY_OUTPUT_DIRECTORY "${LIB_DIR}"
     ARCHIVE_OUTPUT_DIRECTORY "${LIB_DIR}"
)