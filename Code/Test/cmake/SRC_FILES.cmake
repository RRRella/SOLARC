set(${MODULE_NAME}_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(${MODULE_NAME}_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(${MODULE_NAME}_SRC 

${${MODULE_NAME}_SRC_DIR}/Event/EventTest.cpp
${${MODULE_NAME}_SRC_DIR}/Event/EventComponentTest.cpp
${${MODULE_NAME}_SRC_DIR}/Event/EventCellTest.cpp
${${MODULE_NAME}_SRC_DIR}/Event/ObserverEventCellTest.cpp
${${MODULE_NAME}_SRC_DIR}/Event/EventMocks.h
${${MODULE_NAME}_SRC_DIR}/Event/EventMocks.cpp

${${MODULE_NAME}_SRC_DIR}/SolarcAppTest.cpp
${${MODULE_NAME}_SRC_DIR}/main.cpp
${${MODULE_NAME}_SRC_DIR}/FreqUsedSymbolsOfTesting.h
${${MODULE_NAME}_SRC_DIR}/WindowTest.cpp
${${MODULE_NAME}_SRC_DIR}/WindowsWindowTest.cpp

)
