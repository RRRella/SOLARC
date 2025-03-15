set(CMAKE_SYSTEM_NAME Linux)

set(CMAKE_CXX_COMPILER g++)

if (NOT CMAKE_CXX_COMPILER)
    message(FATAL_ERROR "g++ compiler not found!")
endif()

include("./cmake/GCC_FLAGS.cmake")