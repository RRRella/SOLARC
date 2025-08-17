set(${PROJECT_NAME}_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(${PROJECT_NAME}_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(${PROJECT_NAME}_SRC 

${${PROJECT_NAME}_SRC_DIR}/Event/EventTest.cpp
${${PROJECT_NAME}_SRC_DIR}/Event/EventComponentTest.cpp
${${PROJECT_NAME}_SRC_DIR}/Event/EventCellTest.cpp
${${PROJECT_NAME}_SRC_DIR}/Event/ObserverEventCellTest.cpp
${${PROJECT_NAME}_SRC_DIR}/Event/EventFakes.h
${${PROJECT_NAME}_SRC_DIR}/Event/EventFakes.cpp
${${PROJECT_NAME}_SRC_DIR}/Event/EventCommunicationTest.cpp

${${PROJECT_NAME}_SRC_DIR}/MT/ThreadPoolTest.cpp

${${PROJECT_NAME}_SRC_DIR}/main.cpp
${${PROJECT_NAME}_SRC_DIR}/FreqUsedSymbolsOfTesting.h
${${PROJECT_NAME}_SRC_DIR}/WindowTest.cpp
${${PROJECT_NAME}_SRC_DIR}/WindowsWindowTest.cpp

)
