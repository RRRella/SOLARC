cmake_minimum_required(VERSION 3.22)

project(ENERF_CORE LANGUAGES CXX)

#ENerfCore setup
set(MODULE_NAME ENERF_CORE)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/SRC_FILES.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/GROUP_FILES.cmake")

add_library(${MODULE_NAME} SHARED ${${MODULE_NAME}_SRC} ${${MODULE_NAME}_HDRS})

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/DEFS.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/PCH.cmake")

target_include_directories(${MODULE_NAME} PRIVATE ${${MODULE_NAME}_SRC_DIR})
target_include_directories(${MODULE_NAME} PUBLIC ${${MODULE_NAME}_INC_DIR})

set_target_properties( ${MODULE_NAME}
    	PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY "${BIN_DIR}"
	    LIBRARY_OUTPUT_DIRECTORY "${LIB_DIR}"
    	ARCHIVE_OUTPUT_DIRECTORY "${LIB_DIR}"
)