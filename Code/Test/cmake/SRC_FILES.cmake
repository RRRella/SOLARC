set(${PROJECT_NAME}_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(${PROJECT_NAME}_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(${PROJECT_NAME}_SRC 

${${PROJECT_NAME}_SRC_DIR}/Event/ObserverBusTest.cpp
${${PROJECT_NAME}_SRC_DIR}/Event/EventSystemStressTest.cpp

${${PROJECT_NAME}_SRC_DIR}/MT/JobSystemTest.cpp

${${PROJECT_NAME}_SRC_DIR}/main.cpp
${${PROJECT_NAME}_SRC_DIR}/FreqUsedSymbolsOfTesting.h
${${PROJECT_NAME}_SRC_DIR}/WindowTest.cpp

)
