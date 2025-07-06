cmake_minimum_required(VERSION 3.30.2)

project(SOLARC_TEST LANGUAGES CXX C)

include("${CMAKE_SOURCE_DIR}/cmake/GoogleTestSetup.cmake")

#SOLARC_TEST setup
set(MODULE_NAME SOLARC_TEST)

include("${CMAKE_SOURCE_DIR}/cmake/TEST_SETUP.cmake")

include("${CMAKE_SOURCE_DIR}/cmake/PCH.cmake")

target_link_libraries(${MODULE_NAME} PRIVATE 
SOLARC_CORE
gmock
)