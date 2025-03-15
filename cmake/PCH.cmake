target_precompile_headers(${MODULE_NAME} 

PRIVATE "${CMAKE_SOURCE_DIR}/code/PCH.h")

if(MSVC)
    target_compile_options(${MODULE_NAME} PRIVATE "/FI${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${PROJECT_NAME}.dir/$<CONFIG>/cmake_pch.hxx")
endif()