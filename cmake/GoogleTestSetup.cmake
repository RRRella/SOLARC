#Google test setup
option(BUILD_SHARED_LIBS "build google test as shared lib" OFF)
option(gtest_force_shared_crt "Use shared (DLL) run-time lib even when Google Test is built as static lib." OFF)

add_subdirectory(${CMAKE_SOURCE_DIR}/ThirdParty/googletest ${CMAKE_BINARY_DIR}/googletest)

set_target_properties(gmock 	 PROPERTIES
					RUNTIME_OUTPUT_DIRECTORY "${BIN_DIR}"
					LIBRARY_OUTPUT_DIRECTORY "${LIB_DIR}"
					ARCHIVE_OUTPUT_DIRECTORY "${LIB_DIR}")
set_target_properties(gmock_main PROPERTIES
					RUNTIME_OUTPUT_DIRECTORY "${BIN_DIR}"
					LIBRARY_OUTPUT_DIRECTORY "${LIB_DIR}"
					ARCHIVE_OUTPUT_DIRECTORY "${LIB_DIR}")
set_target_properties(gtest 	 PROPERTIES
					RUNTIME_OUTPUT_DIRECTORY "${BIN_DIR}"
					LIBRARY_OUTPUT_DIRECTORY "${LIB_DIR}"
					ARCHIVE_OUTPUT_DIRECTORY "${LIB_DIR}")
set_target_properties(gtest_main PROPERTIES
					RUNTIME_OUTPUT_DIRECTORY "${BIN_DIR}"
					LIBRARY_OUTPUT_DIRECTORY "${LIB_DIR}"
					ARCHIVE_OUTPUT_DIRECTORY "${LIB_DIR}")

option(BUILD_SHARED_LIBS "build google test as shared lib" OFF)
#