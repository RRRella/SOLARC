cmake_minimum_required(VERSION 3.30.2)

project(SOLARC_TEST LANGUAGES CXX)

include("${CMAKE_SOURCE_DIR}/cmake/GoogleTestSetup.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/SRC_FILES.cmake")


add_executable(${PROJECT_NAME} ${${PROJECT_NAME}_SRC} ${${PROJECT_NAME}_HDRS})

include("${CMAKE_SOURCE_DIR}/cmake/GROUP_FILES.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/PCH.cmake")

target_include_directories(${PROJECT_NAME} PRIVATE ${${PROJECT_NAME}_SRC_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${${PROJECT_NAME}_INC_DIR})
target_link_directories(${PROJECT_NAME} PRIVATE "${LIB_DIR}")
target_link_libraries(${PROJECT_NAME} PRIVATE 
SOLARC_CORE
gmock
COMMON_FLAGS
)

set_target_properties( ${PROJECT_NAME}
    	PROPERTIES
    	RUNTIME_OUTPUT_DIRECTORY "${BIN_DIR}"
)

add_custom_command(
    TARGET SOLARC_TEST POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${DATA_DIR}"
        "$<TARGET_FILE_DIR:SOLARC_TEST>/Data"
    COMMENT "Copying data files to output directory"
)