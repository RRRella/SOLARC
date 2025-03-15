set(${MODULE_NAME}_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(${MODULE_NAME}_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(${MODULE_NAME}_SRC 

${${MODULE_NAME}_SRC_DIR}/ENerfApp.cpp
${${MODULE_NAME}_SRC_DIR}/WindowsWindow.cpp

${${MODULE_NAME}_SRC_DIR}/Event/EventComponent.cpp
${${MODULE_NAME}_SRC_DIR}/Event/EventThread.cpp
${${MODULE_NAME}_SRC_DIR}/Event/EventQueue.cpp
${${MODULE_NAME}_SRC_DIR}/Event/EventCell/EventCell.cpp
${${MODULE_NAME}_SRC_DIR}/Event/EventCell/ObserverEventCell.cpp



)

set(${MODULE_NAME}_HDRS

${${MODULE_NAME}_INC_DIR}/ENerfApp.h
${${MODULE_NAME}_INC_DIR}/WindowsWindow.h

${${MODULE_NAME}_INC_DIR}/Event/Event.h
${${MODULE_NAME}_INC_DIR}/Event/EventComponent.h
${${MODULE_NAME}_INC_DIR}/Event/EventThread.h
${${MODULE_NAME}_INC_DIR}/Event/EventQueue.h
${${MODULE_NAME}_INC_DIR}/Event/EventCell/EventCell.h
${${MODULE_NAME}_INC_DIR}/Event/EventCell/ObserverEventCell.h

${${MODULE_NAME}_INC_DIR}/Preprocessor/API.h

${${MODULE_NAME}_INC_DIR}/Utility/UUID.h

)