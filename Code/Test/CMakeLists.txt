cmake_minimum_required(VERSION 3.30.2)

project(ENERF_TEST LANGUAGES CXX C)

include("${CMAKE_SOURCE_DIR}/cmake/GoogleTestSetup.cmake")

#ENERF_TEST setup
set(MODULE_NAME ENERF_TEST)

include("${CMAKE_SOURCE_DIR}/cmake/TEST_SETUP.cmake")

include("${CMAKE_SOURCE_DIR}/cmake/PCH.cmake")

target_link_libraries(${MODULE_NAME} PRIVATE 
ENERF_CORE
gmock
)