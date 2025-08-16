set(${MODULE_NAME}_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(${MODULE_NAME}_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(${MODULE_NAME}_SRC 

${${MODULE_NAME}_SRC_DIR}/SolarcApp.cpp

${${MODULE_NAME}_SRC_DIR}/WindowContext.cpp

${${MODULE_NAME}_SRC_DIR}/Scene.cpp
${${MODULE_NAME}_SRC_DIR}/Renderer.cpp
${${MODULE_NAME}_SRC_DIR}/Window.cpp
${${MODULE_NAME}_SRC_DIR}/WindowsWindow.cpp

${${MODULE_NAME}_SRC_DIR}/Utility/CompileTimeUtil.cpp
${${MODULE_NAME}_SRC_DIR}/Utility/FileSystemUtil.cpp

${${MODULE_NAME}_SRC_DIR}/MT/ThreadPool.cpp


${${MODULE_NAME}_SRC_DIR}/Event/EventQueue.cpp
${${MODULE_NAME}_SRC_DIR}/Event/EventCommunication.cpp
${${MODULE_NAME}_SRC_DIR}/Event/EventCells/ObserverEventCell.cpp
)

set(${MODULE_NAME}_HDRS

${${MODULE_NAME}_INC_DIR}/SolarcApp.h

${${MODULE_NAME}_INC_DIR}/WindowContext.h

${${MODULE_NAME}_INC_DIR}/Scene.h
${${MODULE_NAME}_INC_DIR}/Renderer.h
${${MODULE_NAME}_INC_DIR}/Window.h
${${MODULE_NAME}_INC_DIR}/WindowsWindow.h

${${MODULE_NAME}_INC_DIR}/Event/EventQueue.h
${${MODULE_NAME}_INC_DIR}/Event/Event.h
${${MODULE_NAME}_INC_DIR}/Event/EventCommunication.h
${${MODULE_NAME}_INC_DIR}/Event/EventCells/ObserverEventCell.h

${${MODULE_NAME}_INC_DIR}/Preprocessor/API.h

${${MODULE_NAME}_INC_DIR}/Utility/UUID.h
${${MODULE_NAME}_INC_DIR}/Utility/CompileTimeUtil.h
${${MODULE_NAME}_INC_DIR}/Utility/FileSystemUtil.h

${${MODULE_NAME}_INC_DIR}/MT/ThreadPool.h

)