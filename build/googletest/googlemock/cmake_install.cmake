# Install script for directory: D:/Dev/Local/EnhancedNerf/ThirdParty/googletest/googlemock

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "D:/Program Files (x86)/EnhancedNerf")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "R_DEBUG")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "TRUE")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "gmock" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE DIRECTORY FILES "D:/Dev/Local/EnhancedNerf/ThirdParty/googletest/googlemock/include/")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "gmock" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr]_[Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "D:/Dev/Local/EnhancedNerf/build/out/R_DEBUG/lib/gmock.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr]_[Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "D:/Dev/Local/EnhancedNerf/build/out/R_RELEASE/lib/gmock.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr]_[Pp][Rr][Oo][Ff][Ii][Ll][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "D:/Dev/Local/EnhancedNerf/build/out/R_PROFILE/lib/gmock.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr]_[Tt][Ee][Ss][Tt])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "D:/Dev/Local/EnhancedNerf/build/out/R_TEST/lib/gmock.lib")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "gmock" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr]_[Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "D:/Dev/Local/EnhancedNerf/build/out/R_DEBUG/lib/gmock_main.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr]_[Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "D:/Dev/Local/EnhancedNerf/build/out/R_RELEASE/lib/gmock_main.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr]_[Pp][Rr][Oo][Ff][Ii][Ll][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "D:/Dev/Local/EnhancedNerf/build/out/R_PROFILE/lib/gmock_main.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr]_[Tt][Ee][Ss][Tt])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "D:/Dev/Local/EnhancedNerf/build/out/R_TEST/lib/gmock_main.lib")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "gmock" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr]_[Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE FILE OPTIONAL FILES "D:/Dev/Local/EnhancedNerf/build/bin/${CMAKE_INSTALL_CONFIG_NAME}/gmock.pdb")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr]_[Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE FILE OPTIONAL FILES "D:/Dev/Local/EnhancedNerf/build/bin/${CMAKE_INSTALL_CONFIG_NAME}/gmock.pdb")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr]_[Pp][Rr][Oo][Ff][Ii][Ll][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE FILE OPTIONAL FILES "D:/Dev/Local/EnhancedNerf/build/bin/${CMAKE_INSTALL_CONFIG_NAME}/gmock.pdb")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr]_[Tt][Ee][Ss][Tt])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE FILE OPTIONAL FILES "D:/Dev/Local/EnhancedNerf/build/bin/${CMAKE_INSTALL_CONFIG_NAME}/gmock.pdb")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "gmock" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr]_[Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE FILE OPTIONAL FILES "D:/Dev/Local/EnhancedNerf/build/bin/${CMAKE_INSTALL_CONFIG_NAME}/gmock_main.pdb")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr]_[Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE FILE OPTIONAL FILES "D:/Dev/Local/EnhancedNerf/build/bin/${CMAKE_INSTALL_CONFIG_NAME}/gmock_main.pdb")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr]_[Pp][Rr][Oo][Ff][Ii][Ll][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE FILE OPTIONAL FILES "D:/Dev/Local/EnhancedNerf/build/bin/${CMAKE_INSTALL_CONFIG_NAME}/gmock_main.pdb")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr]_[Tt][Ee][Ss][Tt])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE FILE OPTIONAL FILES "D:/Dev/Local/EnhancedNerf/build/bin/${CMAKE_INSTALL_CONFIG_NAME}/gmock_main.pdb")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "gmock" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig" TYPE FILE FILES "D:/Dev/Local/EnhancedNerf/build/googletest/googletest/generated/gmock.pc")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "gmock" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig" TYPE FILE FILES "D:/Dev/Local/EnhancedNerf/build/googletest/googletest/generated/gmock_main.pc")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("D:/Dev/Local/EnhancedNerf/build/googletest/googletest/cmake_install.cmake")

endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "D:/Dev/Local/EnhancedNerf/build/googletest/googlemock/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
