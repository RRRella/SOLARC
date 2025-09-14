set(${PROJECT_NAME}_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(${PROJECT_NAME}_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(${PROJECT_NAME}_SRC 

${${PROJECT_NAME}_SRC_DIR}/SolarcApp.cpp

${${PROJECT_NAME}_SRC_DIR}/WindowContext.cpp

${${PROJECT_NAME}_SRC_DIR}/Scene.cpp
${${PROJECT_NAME}_SRC_DIR}/Renderer.cpp
${${PROJECT_NAME}_SRC_DIR}/Window.cpp
${${PROJECT_NAME}_SRC_DIR}/WindowsWindowPlatform.cpp

${${PROJECT_NAME}_SRC_DIR}/Utility/CompileTimeUtil.cpp
${${PROJECT_NAME}_SRC_DIR}/Utility/FileSystemUtil.cpp

${${PROJECT_NAME}_SRC_DIR}/MT/ThreadPool.cpp


${${PROJECT_NAME}_SRC_DIR}/Event/EventQueue.cpp
${${PROJECT_NAME}_SRC_DIR}/Event/EventCommunication.cpp
${${PROJECT_NAME}_SRC_DIR}/Event/EventCells/ObserverEventCell.cpp
)

set(${PROJECT_NAME}_HDRS

${${PROJECT_NAME}_INC_DIR}/SolarcApp.h

${${PROJECT_NAME}_INC_DIR}/WindowContext.h

${${PROJECT_NAME}_INC_DIR}/Scene.h
${${PROJECT_NAME}_INC_DIR}/Renderer.h
${${PROJECT_NAME}_INC_DIR}/Window.h
${${PROJECT_NAME}_INC_DIR}/WindowsWindowPlatform.h

${${PROJECT_NAME}_INC_DIR}/Event/EventQueue.h
${${PROJECT_NAME}_INC_DIR}/Event/Event.h
${${PROJECT_NAME}_INC_DIR}/Event/EventCommunication.h
${${PROJECT_NAME}_INC_DIR}/Event/EventCells/ObserverEventCell.h

${${PROJECT_NAME}_INC_DIR}/Preprocessor/API.h

${${PROJECT_NAME}_INC_DIR}/Utility/UUID.h
${${PROJECT_NAME}_INC_DIR}/Utility/CompileTimeUtil.h
${${PROJECT_NAME}_INC_DIR}/Utility/FileSystemUtil.h

${${PROJECT_NAME}_INC_DIR}/MT/ThreadPool.h

)