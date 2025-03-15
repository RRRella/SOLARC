include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/SRC_FILES.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/GROUP_FILES.cmake")

add_executable(${MODULE_NAME} ${${MODULE_NAME}_SRC} ${${MODULE_NAME}_HDRS})

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/DEFS.cmake")

target_include_directories(${MODULE_NAME} PRIVATE ${${MODULE_NAME}_SRC_DIR})
target_include_directories(${MODULE_NAME} PRIVATE ${${MODULE_NAME}_INC_DIR})

target_link_directories(${MODULE_NAME} PRIVATE "${LIB_DIR}")


set_target_properties( ${MODULE_NAME}
    	PROPERTIES
    	RUNTIME_OUTPUT_DIRECTORY "${BIN_DIR}"
)