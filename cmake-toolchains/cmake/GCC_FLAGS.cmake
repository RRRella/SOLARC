set(CMAKE_CXX_FLAGS "-fstack-protector-strong -fPIC -faligned-new")
set(CMAKE_CXX_FLAGS_R_DEBUG "-g -O0 ")
set(CMAKE_CXX_FLAGS_R_RELEASE "-O2")
set(CMAKE_CXX_FLAGS_R_PROFILE "-g -O2")
set(CMAKE_CXX_FLAGS_R_TEST "-g -O0")

set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--gc-sections -Wl,--icf=all -Wl,-z,norelro /SUBSYSTEM:CONSOLE ")
set(CMAKE_EXE_LINKER_FLAGS "-Wl,--gc-sections -Wl,--icf=all -Wl,-z,norelro /SUBSYSTEM:CONSOLE ")

set(CMAKE_SHARED_LINKER_FLAGS_R_RELEASE "-flto")
set(CMAKE_EXE_LINKER_FLAGS_R_RELEASE    "-flto")

set(CMAKE_SHARED_LINKER_FLAGS_R_DEBUG "-Wl,-v -g -Wl,-Map")
set(CMAKE_EXE_LINKER_FLAGS_R_DEBUG    "-Wl,-v -g -Wl,-Map")

set(CMAKE_SHARED_LINKER_FLAGS_R_PROFILE "-pg")
set(CMAKE_EXE_LINKER_FLAGS_R_PROFILE "-pg")

set(CMAKE_SHARED_LINKER_FLAGS_R_TEST "-g")
set(CMAKE_EXE_LINKER_FLAGS_R_TEST "-g ")
