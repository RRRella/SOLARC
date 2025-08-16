set(${MODULE_NAME}_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(${MODULE_NAME}_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(${MODULE_NAME}_SRC 

${${MODULE_NAME}_SRC_DIR}/Event/EventTest.cpp
${${MODULE_NAME}_SRC_DIR}/Event/EventComponentTest.cpp
${${MODULE_NAME}_SRC_DIR}/Event/EventCellTest.cpp
${${MODULE_NAME}_SRC_DIR}/Event/ObserverEventCellTest.cpp
${${MODULE_NAME}_SRC_DIR}/Event/EventFakes.h
${${MODULE_NAME}_SRC_DIR}/Event/EventFakes.cpp
${${MODULE_NAME}_SRC_DIR}/Event/EventCommunicationTest.cpp

${${MODULE_NAME}_SRC_DIR}/MT/ThreadPoolTest.cpp

${${MODULE_NAME}_SRC_DIR}/main.cpp
${${MODULE_NAME}_SRC_DIR}/FreqUsedSymbolsOfTesting.h
${${MODULE_NAME}_SRC_DIR}/WindowTest.cpp
${${MODULE_NAME}_SRC_DIR}/WindowsWindowTest.cpp

)
