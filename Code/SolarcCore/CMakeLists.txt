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

target_include_directories(${PROJECT_NAME} PRIVATE ${${PROJECT_NAME}_SRC_DIR})
target_include_directories(${PROJECT_NAME} PUBLIC ${${PROJECT_NAME}_INC_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog COMMON_FLAGS)

set_target_properties( ${PROJECT_NAME}
    	PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY "${BIN_DIR}"
	    LIBRARY_OUTPUT_DIRECTORY "${LIB_DIR}"
    	ARCHIVE_OUTPUT_DIRECTORY "${LIB_DIR}"
)