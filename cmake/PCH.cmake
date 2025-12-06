target_precompile_headers(${PROJECT_NAME}
    PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_SOURCE_DIR}/Code/PCH.h>
)


if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE "/FI${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${PROJECT_NAME}.dir/$<CONFIG>/cmake_pch.hxx")
endif()

