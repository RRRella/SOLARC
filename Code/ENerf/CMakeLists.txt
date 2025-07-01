cmake_minimum_required(VERSION 3.22)

project(ENERF LANGUAGES CXX)

#ENERF_EDITOR setup
set(MODULE_NAME ENERF)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/SRC_FILES.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/GROUP_FILES.cmake")

add_executable(${MODULE_NAME} ${${MODULE_NAME}_SRC} ${${MODULE_NAME}_HDRS})

add_compile_options("/ENTRY:wWinMainCRTStartup")

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/DEFS.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/PCH.cmake")

target_include_directories(${MODULE_NAME} PRIVATE ${${MODULE_NAME}_SRC_DIR})
target_include_directories(${MODULE_NAME} PUBLIC ${${MODULE_NAME}_INC_DIR})

target_link_directories(${MODULE_NAME} PRIVATE ${LIB_DIR})

target_link_libraries(${MODULE_NAME}

PRIVATE ENERF_CORE

)

set_target_properties( ${MODULE_NAME}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${BIN_DIR}"
)

add_custom_command(
    TARGET ENERF POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${DATA_DIR}"
        "$<TARGET_FILE_DIR:ENERF>/Data"
    COMMENT "Copying data files to output directory"
)